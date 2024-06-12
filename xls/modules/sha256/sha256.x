// bit rotate right
fn rot_right(x: u32, n: u32) -> u32 {
    (x >> n) | (x << (u32:32 - n))
}

#[test]
fn test_rot_right() {
    // zero bit rotate
    let x1 = u32:0b10101010101010101010101010101010;
    assert_eq(rot_right(x1, u32:0), x1);

    // full bit rotate
    let x2 = u32:0b10101010101010101010101010101010;
    assert_eq(rot_right(x2, u32:32), x2);

    // partial rotate
    let x3 = u32:0b00000000000000000000000000000001;
    let ans3 = u32:0b10000000000000000000000000000000;
    assert_eq(rot_right(x3, u32:1), ans3);

    // partial rotate
    let x4 = u32:0b00101010001010100010101000101011;
    let ans4 = u32:0b01100101010001010100010101000101;
    assert_eq(rot_right(x4, u32:3), ans4);
}


// Choose function
fn ch(x: u32, y: u32, z: u32) -> u32 {
    (x & y) ^ (!x & z)
}

#[test]
fn test_ch() {
    // x is all 1s
    let x1 = u32:0b11111111111111111111111111111111;
    let y1 = u32:0b10101010101010101010101010101010;
    let z1 = u32:0b01010101010101010101010101010101;
    assert_eq(ch(x1, y1, z1), y1);

    // x is all 0s
    let x2 = u32:0b00000000000000000000000000000000;
    let y2 = u32:0b10101010101010101010101010101010;
    let z2 = u32:0b01010101010101010101010101010101;
    assert_eq(ch(x2, y2, z2), z2);

    // normal case
    let x3 = u32:0b01010101010101011010101010101010;
    let y3 = u32:0b10101010101010101010101010101010;
    let z3 = u32:0b01010101010101010101010101010101;
    let ans3 = u32:0b00000000000000001111111111111111;
    assert_eq(ch(x3, y3, z3), ans3);
}

// Majority function
fn maj(x: u32, y: u32, z: u32) -> u32 {
    (x & y) ^ (x & z) ^ (y & z)
}

#[test]
fn test_maj() {
    let x1 = u32:0b00000000000000000000000000000000;
    let y1 = u32:0b10101010101010101010101010101010;
    let z1 = u32:0b01010101010101010101010101010101;
    let ans1 = u32:0b00000000000000000000000000000000;
    assert_eq(maj(x1, y1, z1), ans1);

    let x2 = u32:0b10101010101010101010101010101010;
    let y2 = u32:0b10101010101010101010101010101010;
    let z2 = u32:0b01010101010101010101010101010101;
    let ans2 = u32:0b10101010101010101010101010101010;
    assert_eq(maj(x2, y2, z2), ans2);
}

// big sigma0 function
fn sigma_big_0(x: u32) -> u32 {
    rot_right(x, 2) ^ rot_right(x, 13) ^ rot_right(x, 22)
}

// big sigma1 function
fn sigma_big_1(x: u32) -> u32 {
    rot_right(x, 6) ^ rot_right(x, 11) ^ rot_right(x, 25)
}

// small sigma0 function
fn sigma_small_0(x: u32) -> u32 {
    rot_right(x, 7) ^ rot_right(x, 18) ^ (x >> 3)
}

// small sigma1 function
fn sigma_small_1(x: u32) -> u32 {
    rot_right(x, 17) ^ rot_right(x, 19) ^ (x >> 10)
}


// padding function
proc Padding {
    input_data: chan<uN[512]> in;
    input_data_total_length: chan<uN[64]> in;
    input_data_length_in_this_cycle: chan<uN[10]> in;
    output_padded_data: chan<uN[512]> out;

    init {
        uN[1]:0
    }

    config(input_data: chan<uN[512]> in, input_data_total_length: chan<uN[64]> in, input_data_length_in_this_cycle: chan<uN[9]> in, output_padded_data: chan<uN[512]> out) {
        (input_data, input_data_total_length, input_data_length_in_this_cycle, output_padded_data)
    }

    next(tok: token, : uN[1]) {
        let (tok_data, data) = recv(tok, input_data);
        let (tok_data_total_length, data_total_length) = recv(tok, input_data_total_length);
        let (tok_data_length_in_this_cycle, data_length_in_this_cycle) = recv(tok, input_data_length_in_this_cycle);
        let tok_all = join(tok_data, tok_data_total_length, tok_data_length_in_this_cycle);

    }
}
