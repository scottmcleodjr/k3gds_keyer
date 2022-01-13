/*
 * v1.0 CW Keyer by Scott K3GDS 
 */

#define MEMORY_SIZE 1500
#define DIT 1
#define DAH 2
#define CHAR_SPACE 3
#define WORD_SPACE 4

// Char and word space are 1 down because they are
// always followed by a space in a dit or dah
const int DIT_UNITS   = 1;
const int DAH_UNITS   = 3;
const int SPACE_UNITS = 1;
const int CHAR_SPACE_UNITS = 2;
const int WORD_SPACE_UNITS = 6;

const int LEFT_INPUT  = 4;
const int RIGHT_INPUT = 2; 
const int SPEED_POT   = A0;
const int OUT_PIN     = 12;
const int MEMORY_LED_PIN    = 13;
const int MEMORY_BUTTON_PIN = 8;

int unit_length_ms;
int dit_length_ms;
int dah_length_ms;
int space_length_ms;
int char_space_length_ms;
int word_space_length_ms;

boolean is_recording;
byte last_played;
byte left_state;
byte right_state;
byte memory_button_state; 
int button_duration;
unsigned long silence_begin_time;

byte memory[MEMORY_SIZE];
int memory_index;

void memory_add_item(int event) {
	if (memory_index == MEMORY_SIZE) {
		stop_recording();
	} else if (memory_index == 0 && (event == CHAR_SPACE || event == WORD_SPACE)) {
		// Do nothing.  Don't add leading silence.
	} else {
		memory[memory_index] = event;
		memory_index++;
	} 
}

void memory_add_silence(int duration) {
	if (duration < (dah_length_ms * 0.5)) {
		// Do nothing.
	} else if (duration >= (dah_length_ms * 0.5) && duration <= (dah_length_ms * 1.5)) {
		memory_add_item(CHAR_SPACE);
	} else {
		memory_add_item(WORD_SPACE);
	}
}

void play_memory() {
	memory_index = 0;
	while (memory[memory_index] != 42 && memory_index < MEMORY_SIZE) {
		if (!digitalRead(LEFT_INPUT) || !digitalRead(RIGHT_INPUT)) {
			break;  
		}
		set_lengths();
	
		byte next_item = memory[memory_index];
		switch (next_item) {
			case DIT:
				play(DIT);
				break;
			case DAH:
				play(DAH);
				break;
			case CHAR_SPACE:
				delay(char_space_length_ms);
				break;
			case WORD_SPACE:
				delay(word_space_length_ms);
				break;
		}
		memory_index++;
	}
}

void clean_memory() {
	for(int x = 0; x < MEMORY_SIZE; x++) {
  		memory[x] = 42; // A meaningful number
	}  
}

void start_recording() {
	clean_memory();
	memory_index = 0;
	is_recording = true;
	digitalWrite(MEMORY_LED_PIN, HIGH);
}

void stop_recording() {
	is_recording = false;
	digitalWrite(MEMORY_LED_PIN, LOW);
}

void play(int char_type) {
	if (is_recording) {
		if (silence_begin_time != 0) {
			memory_add_silence(millis() - silence_begin_time);
		}
		memory_add_item(char_type);
	}

	delay(space_length_ms);
	digitalWrite(OUT_PIN, HIGH);
	if (char_type == DIT) {
		delay(dit_length_ms);
	} else if (char_type == DAH) {
		delay(dah_length_ms);
	}
	digitalWrite(OUT_PIN, LOW);

	last_played = char_type;
	silence_begin_time = 0;
}

void set_lengths() {
	/* The math in the first line covers pot input from 0 to 1023.
	 * This gives values from 40 - 80ms.
	 */
	unit_length_ms = 40 + (analogRead(SPEED_POT) / 25);
	dit_length_ms   = unit_length_ms * DIT_UNITS;
	dah_length_ms   = unit_length_ms * DAH_UNITS;
	space_length_ms = unit_length_ms * SPACE_UNITS;
	char_space_length_ms = unit_length_ms * CHAR_SPACE_UNITS;
	word_space_length_ms = unit_length_ms * WORD_SPACE_UNITS;
}

void setup() {
	pinMode(LEFT_INPUT, INPUT_PULLUP);
	pinMode(RIGHT_INPUT, INPUT_PULLUP);
	pinMode(MEMORY_LED_PIN, OUTPUT);
	pinMode(MEMORY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(OUT_PIN, OUTPUT);
	set_lengths();
	last_played = DIT; // Could also be DAH
	is_recording = false;
}

void loop() {
	set_lengths();

	// Not reversed becaues my button is normally closed type
	memory_button_state = digitalRead(MEMORY_BUTTON_PIN);
	if (memory_button_state) {
		button_duration = 0;
		while (memory_button_state) {
			delay(10);
			button_duration += 10;
			// Not reversed because my button is normally closed type
			memory_button_state = digitalRead(MEMORY_BUTTON_PIN);
		}
		if (is_recording) {
			stop_recording();
		} else if (button_duration > 1000) {
			start_recording();
		} else {
			play_memory();
		}
	}
  
	// !Value beause INPUT_PULLUP in setup()
	left_state  = !digitalRead(LEFT_INPUT);
	right_state = !digitalRead(RIGHT_INPUT);

	if ((!left_state) && (!right_state)) {
		if (silence_begin_time == 0) { // Silence not most recent
			silence_begin_time = millis();
		}
	}

	if (left_state && (!right_state)) {
		play(DIT);
	}
	if ((!left_state) && right_state) {
		play(DAH);
	}
	if (left_state && right_state) {
		if (last_played == DIT) {
			play(DAH);
		}
		if (last_played == DAH) {
			play(DIT);
		}
	}
}
