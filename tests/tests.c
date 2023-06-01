/*!
 * @file /tests.c
 * @brief Tests of Simpio internal utility functions
 * @details
 * This contains tests of functions of Simpio that are not easy to
 * test thoroughly through PIO programs. 
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */
 
void test_shift_get_bit() {
    bool bit;
    uint32_t c = 0x10820404; //3 zeros 1, then 4 zeros 1, then 5 zeros 1, then 6 zeros 1, then 7 zeros 1, then 2 zeros
    uint32_t s,original;
    int i, direction;
    s = c;
    printf("shift right (LSB first): ");
    print_zero_pattern(s, 1);
    printf(" => ");
    for (i=0; i<32; i++) {
        bit = shift_get_bit(&s,1);
        if (bit) printf("1 ");
        else printf("0");
    }
    s = c;
    printf("\nshift left (MSB first): ");
    print_zero_pattern(s,0);
    printf(" => ");
    for (i=0; i<32; i++) {
        bit = shift_get_bit(&s,0);
        if (bit) printf(" 1");
        else printf("0");
    }
    printf("\n");
}

// Shifting to the right means starting with the rightmost value(s) first
// As they are placed on the left and moved to the right, they will end up on the right.
// And visa versa for shifting to the left.
void test_shift_into() {
    uint32_t c = 0x12345678; // 0001 0010 0011 0100 0101 0110 0111 1000
    uint32_t target;
    int i;
    // first shift c into t to the right 
    target = 0;
    
    shift_into(1, &target, 1);
    
    shift_into(1, &target, 1);
    shift_into(1, &target, 1);
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    
    shift_into(1, &target, 0);
    shift_into(1, &target, 1);
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    
    shift_into(1, &target, 0);
    shift_into(1, &target, 0);
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    
    shift_into(1, &target, 1);
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    shift_into(1, &target, 0);
    
    shift_into(1, &target, 0);
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    shift_into(1, &target, 0);
    
    shift_into(1, &target, 1);
    shift_into(1, &target, 0);
    shift_into(1, &target, 0);
    shift_into(1, &target, 0);
    
    
    if (target != c) printf("shift into right fail: %08X != %08X\n", c, target);
    else printf ("test_shift_into test passes in the right direction\n");
    
    // 0001 0010 0011 0100 0101 0110 0111 1000, now to the left
    target = 0;
    
    shift_into(0, &target, 1);
    
    shift_into(0, &target, 0);
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    shift_into(0, &target, 0);
    
    shift_into(0, &target, 0);
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    shift_into(0, &target, 1);
    
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    shift_into(0, &target, 0);
    shift_into(0, &target, 0);
    
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    shift_into(0, &target, 1);
    shift_into(0, &target, 0);
    
    shift_into(0, &target, 0);
    shift_into(0, &target, 1);
    shift_into(0, &target, 1);
    shift_into(0, &target, 1);
    
    shift_into(0, &target, 1);
    shift_into(0, &target, 0);
    shift_into(0, &target, 0);
    shift_into(0, &target, 0);
    
    if (target != c) printf("shift into left fail: %08X != %08X\n", c, target);
    else printf ("test_shift_into test passes in the left direction\n");
}

