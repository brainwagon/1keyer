#define NSYMS		(40)

const uint8_t pattab [NSYMS] PROGMEM = {
	// letters are the first 26 characters
	6,		// A - dit dah
	17,		// B - dah dit dit dit
	21,		// C - dah dit dah dit
	9,		// D - dah dit dit
	2,		// E - dit
	20,		// F
	11,		// G
	16,		// H
	4,		// I
	30,		// J
	13,		// K
	18,		// L
	7,		// M
	5,		// N
	15,		// O
	22,		// P
	27,		// Q
	10,		// R
	8,		// S
	3,		// T
	12,		// U
	24,		// V
	14,		// W
	25,		// X
	29,		// Y
	19,		// Z
	// Now the patterns for the digits...
	63,		// 0
	62,		// 1
	60,		// 2
	56,		// 3
	48,		// 4
	32,		// 5
	33,		// 6
	35,		// 7
	39,		// 8
	47,		// 9
	// And some misc patterns... 
  	106,		// period
	115,		// comma
	76,		// questionmark
	41		// slash
} ;

const uint8_t chartab[NSYMS] PROGMEM = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'.', ',', '?', '/'
} ;
