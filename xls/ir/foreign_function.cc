// Copyright 2023 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/ir/foreign_function.h"

#include <cstdint>
#include <stack>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/foreign_function_data.pb.h"
#include "re2/re2.h"

namespace xls {

absl::StatusOr<CodeTemplate> CodeTemplate::Create(
    std::string_view template_text) {
  CodeTemplate new_instance;
  XLS_RETURN_IF_ERROR(new_instance.Parse(template_text));
  return new_instance;
}

static absl::Status TemplateParseError(int64_t col, std::string_view message) {
  return absl::InvalidArgumentError(absl::StrFormat("%d: %s", col, message));
}

/*static*/ int64_t CodeTemplate::ExtractErrorColumn(const absl::Status& s) {
  static const RE2 sColExtractRE("^([0-9]+):");
  int64_t result = 0;  // Fallback if there is no column.
  RE2::PartialMatch(s.message(), sColExtractRE, &result);
  return result;
}

absl::Status CodeTemplate::Parse(std::string_view template_text) {
  // Keep track of nesting and also record column for error reporting.
  std::stack<int64_t> paren_opened_at_column;
  std::stack<int64_t> brace_opened_at_column;

  // Expressions are surrounded by braces, but can contain nested
  // braces that should be kept inside as-is. So keep track of the actual
  // nest level we expect at the end of an expression.
  int64_t expected_expression_brace_nest = 0;
  enum class State { kInText, kBraceSeen, kInExpr } state = State::kInText;
  std::string_view::const_iterator start_of_text = template_text.begin();
  std::string_view::const_iterator start_of_expression;
  std::string_view::const_iterator pos;
  for (pos = template_text.begin(); pos != template_text.end(); ++pos) {
    const int64_t col = pos - template_text.begin();
    switch (*pos) {
      // General nesting book-keeping.
      case '(':
        paren_opened_at_column.push(col);
        break;
      case ')':
        if (paren_opened_at_column.empty()) {
          return TemplateParseError(col, "Too many closing parentheses");
        }
        paren_opened_at_column.pop();
        break;
      case '{':
        brace_opened_at_column.push(col);
        break;
      case '}':
        if (brace_opened_at_column.empty()) {
          return TemplateParseError(col, "Too many closing braces");
        }
        brace_opened_at_column.pop();
        break;
    }

    switch (state) {
      case State::kInText: {
        if (*pos == '{') {
          state = State::kBraceSeen;
        }
        break;
      }
      case State::kBraceSeen: {
        if (*pos == '{') {
          state = State::kInText;  // Escaped '{'
        } else {
          start_of_expression = pos;
          expected_expression_brace_nest = brace_opened_at_column.size() - 1;
          leading_text_.emplace_back(start_of_text, pos - 1);
          if (*pos == '}') {  // Immediately closed empty expression
            expressions_.emplace_back(start_of_expression, pos);
            start_of_text = pos + 1;
            state = State::kInText;
          } else {
            state = State::kInExpr;
          }
        }
        break;
      }
      case State::kInExpr: {
        if (*pos == '}' &&
            expected_expression_brace_nest == brace_opened_at_column.size()) {
          expressions_.emplace_back(start_of_expression, pos);
          start_of_text = pos + 1;
          state = State::kInText;
        }
        break;
      }
    }
  }

  if (state == State::kBraceSeen) {
    return TemplateParseError(brace_opened_at_column.top(),
                              "Dangling opened {");
  }

  if (state == State::kInExpr) {
    return TemplateParseError(brace_opened_at_column.top(),
                              "Template expression not closed");
  }

  if (start_of_text < pos) {
    leading_text_.emplace_back(start_of_text, pos);
  }

  if (!brace_opened_at_column.empty()) {
    return TemplateParseError(brace_opened_at_column.top(),
                              "Brace opened here missing closing '}'");
  }

  if (!paren_opened_at_column.empty()) {
    return TemplateParseError(
        paren_opened_at_column.top(),
        "Parenthesis opened here missing closing ')' (xkcd/859)");
  }

  return absl::OkStatus();
}

// Unescape "{{" -> "{" and "}}" -> "}" if unescaping requested.
static std::string UnescapeCurly(bool do_unescape, std::string_view in) {
  std::string result;
  if (!do_unescape) {
    result = in;
    return result;
  }
  result.reserve(in.size());
  char previous = '\0';
  for (char c : in) {
    if (previous == '{' || previous == '}') {
      previous = '\0';  // allow multi escapes {{{{
      continue;
    }
    result.append(1, c);
    previous = c;
  }
  return result;
}

absl::StatusOr<std::string> CodeTemplate::FillTemplate(
    absl::Span<const std::string> replacements, bool escape_curly,
    std::string_view expression_prefix,
    std::string_view expression_suffix) const {
  if (replacements.size() != expressions_.size()) {
    return absl::InvalidArgumentError("Invalid count of {...} replacements.");
  }
  std::string result;
  for (int i = 0; i < expressions_.size(); ++i) {
    absl::StrAppend(&result, UnescapeCurly(!escape_curly, leading_text_[i]),
                    expression_prefix, replacements[i], expression_suffix);
  }
  if (leading_text_.size() > expressions_.size()) {
    absl::StrAppend(&result,
                    UnescapeCurly(!escape_curly, leading_text_.back()));
  }
  return result;
}

absl::StatusOr<std::string> CodeTemplate::FillTemplate(
    absl::Span<const std::string> replacements) const {
  return FillTemplate(replacements, false, "", "");
}

absl::StatusOr<std::string> CodeTemplate::FillEscapedTemplate(
    absl::Span<const std::string> replacements) const {
  return FillTemplate(replacements, true, "{", "}");
}

std::string CodeTemplate::ToString() const {
  absl::StatusOr<std::string> result = FillEscapedTemplate(expressions_);
  XLS_CHECK_OK(result.status());  // Should never happen: #expr count correct.
  return *result;
}

absl::StatusOr<ForeignFunctionData> ForeignFunctionDataCreateFromTemplate(
    std::string_view annotation) {
  absl::StatusOr<CodeTemplate> parse_result = CodeTemplate::Create(annotation);
  if (!parse_result.ok()) {
    return parse_result.status();
  }
  // We just pass the template along as string, but we validated it worked
  ForeignFunctionData result;
  result.set_code_template(annotation);
  return result;
}

}  // namespace xls
