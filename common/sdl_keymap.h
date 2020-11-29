/*
** Standard US ANSI keyboard layout for SDL
*/

// covers all ASCII characters producible from a standard ANSI/ISO keyboard
#define KEYMAP_SIZE 105

static KeyASCIIType sdl_keymap[KEYMAP_SIZE * 2] = {
    // 0
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_a,
    KA_b,
    KA_c,
    KA_d,
    KA_e,
    KA_f,

    // 10
    KA_g,
    KA_h,
    KA_i,
    KA_j,
    KA_k,
    KA_l,
    KA_m,
    KA_n,
    KA_o,
    KA_p,

    // 20
    KA_q,
    KA_r,
    KA_s,
    KA_t,
    KA_u,
    KA_v,
    KA_w,
    KA_x,
    KA_y,
    KA_z,

    // 30
    KA_1,
    KA_2,
    KA_3,
    KA_4,
    KA_5,
    KA_6,
    KA_7,
    KA_8,
    KA_9,
    KA_0,

    // 40
    KA_RETURN,
    KA_ESC,
    KA_BACKSPACE,
    KA_TAB,
    KA_SPACE,
    KA_MINUS,
    KA_EQUAL,
    KA_LBRACKET,
    KA_RBRACKET,
    KA_BACKSLASH,

    // 50
    KA_NONE,
    KA_SEMICOLON,
    KA_SQUOTE,
    KA_GRAVE,
    KA_COMMA,
    KA_PERIOD,
    KA_SLASH,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 60
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 70
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 80
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_SLASH,
    KA_ASTERISK,
    KA_MINUS,
    KA_PLUS,
    KA_RETURN,
    KA_1,

    // 90
    KA_2,
    KA_3,
    KA_4,
    KA_5,
    KA_6,
    KA_7,
    KA_8,
    KA_9,
    KA_0,
    KA_PERIOD,

    // 100
    KA_BACKSLASH, // ISO
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    /*
    ** Shifted keys
    */

    // 0
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_A,
    KA_B,
    KA_C,
    KA_D,
    KA_E,
    KA_F,

    // 10
    KA_G,
    KA_H,
    KA_I,
    KA_J,
    KA_K,
    KA_L,
    KA_M,
    KA_N,
    KA_O,
    KA_P,

    // 20
    KA_Q,
    KA_R,
    KA_S,
    KA_T,
    KA_U,
    KA_V,
    KA_W,
    KA_X,
    KA_Y,
    KA_Z,

    // 30
    KA_EXCLAMATION,
    KA_AT,
    KA_POUND,
    KA_DOLLAR,
    KA_PERCENT,
    KA_CARROT,
    KA_AMPER,
    KA_ASTERISK,
    KA_LPAREN,
    KA_RPAREN,

    // 40
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_UNDERLINE,
    KA_PLUS,
    KA_LBRACE,
    KA_RBRACE,
    KA_BAR,

    // 50
    KA_NONE,
    KA_COLON,
    KA_DQUOTE,
    KA_TILDA,
    KA_LESS_THAN,
    KA_GREATER_THAN,
    KA_QUESTION,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 60
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 70
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 80
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 90
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE,

    // 100
    KA_BAR, // ISO
    KA_NONE,
    KA_NONE,
    KA_NONE,
    KA_NONE};
