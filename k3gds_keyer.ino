/*
 * v1.2 CW Keyer by Scott K3GDS
 */

#define DEBUG 0
#if DEBUG
  #define debug(...)   Serial.print(__VA_ARGS__)
  #define debugln(...) Serial.println(__VA_ARGS__)
#else
  #define debug(...)
  #define debugln(...)
#endif

#define MEMORY_END 255
#define DIT 0
#define DAH 1
#define CHAR_SPACE 2
#define WORD_SPACE 3

typedef byte Event;

const int MEMORY_SIZE = 1500;
const int LEFT_INPUT = 4;
const int RIGHT_INPUT = 2;
const int SPEED_POT = A0;
const int OUT_PIN = 12;
const int MEMORY_LED_PIN = 13;
const int MEMORY_BUTTON_PIN = 8;
const int DIT_UNITS = 1;
const int DAH_UNITS = 3;
const int SPACE_UNITS = 1;
const int CHAR_SPACE_UNITS = 2;
const int WORD_SPACE_UNITS = 6;

boolean is_recording = false;
Event memory[MEMORY_SIZE] = {MEMORY_END};
Event last_played = DIT;
int memory_index = 0;
unsigned long silence_begin_time = 0;

void setup()
{
  #if DEBUG
    Serial.begin(9600);
  #endif

  pinMode(LEFT_INPUT, INPUT_PULLUP);
  pinMode(RIGHT_INPUT, INPUT_PULLUP);
  pinMode(MEMORY_LED_PIN, OUTPUT);
  pinMode(MEMORY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OUT_PIN, OUTPUT);
}

void loop()
{
  int memory_button_duration = 0;
  while (digitalRead(MEMORY_BUTTON_PIN))
  {
    delay(10);
    memory_button_duration += 10;
  }
  if (memory_button_duration && is_recording)
  {
    stop_recording();
  }
  else if (memory_button_duration > 1000)
  {
    start_recording();
  }
  else if (memory_button_duration)
  {
    play_memory();
  }

  // !Value beause INPUT_PULLUP in setup()
  char left_state = !digitalRead(LEFT_INPUT);
  char right_state = !digitalRead(RIGHT_INPUT);

  if (!left_state && !right_state && silence_begin_time == 0)
  {
    silence_begin_time = millis();
  }

  if (left_state && (!right_state))
  {
    play(DIT);
  }
  if ((!left_state) && right_state)
  {
    play(DAH);
  }
  if (left_state && right_state && last_played == DIT)
  {
    play(DAH);
  }
  if (left_state && right_state && last_played == DAH)
  {
    play(DIT);
  }
}

void play(Event e)
{
  boolean is_space = e == CHAR_SPACE || e == WORD_SPACE;

  if (is_recording && silence_begin_time != 0)
  {
    memory_add_silence(millis() - silence_begin_time);
  }
  if (is_recording && !is_space)
  {
    memory_add_event(e);
  }

  // Space between dits is same length as a dit
  delay(event_length_ms(DIT));

  if (!is_space)
  {
    digitalWrite(OUT_PIN, HIGH);
  }
  delay(event_length_ms(e));
  if (!is_space)
  {
    digitalWrite(OUT_PIN, LOW);
  }

  last_played = e;
  silence_begin_time = 0;
}

void play_memory()
{
  memory_index = 0;
  while (memory[memory_index] != MEMORY_END && memory_index < MEMORY_SIZE)
  {
    if (!digitalRead(LEFT_INPUT) || !digitalRead(RIGHT_INPUT))
    {
      break;
    }
    play(memory[memory_index]);
    memory_index++;
  }
}

void start_recording()
{
  memory_index = 0;
  is_recording = true;
  digitalWrite(MEMORY_LED_PIN, HIGH);
}

void stop_recording()
{
  memory[memory_index] = MEMORY_END;
  is_recording = false;
  digitalWrite(MEMORY_LED_PIN, LOW);
}

int event_length_ms(Event e)
{
  /* The math in the first line covers pot input from 0 to 1023.
   * This gives values from 40 - 80ms.
   */
  int unit_length_ms = 40 + (analogRead(SPEED_POT) / 25);
  switch (e)
  {
  case DIT:
    return unit_length_ms * DIT_UNITS;
  case DAH:
    return unit_length_ms * DAH_UNITS;
  case CHAR_SPACE:
    return unit_length_ms * CHAR_SPACE;
  case WORD_SPACE:
    return unit_length_ms * WORD_SPACE;
  default:
    return 0;
  }
}

void memory_add_event(Event e)
{
  // - 1 becaue we'll set this index in stop_recording
  if (memory_index == MEMORY_SIZE - 1)
  {
    stop_recording();
  }
  else if (memory_index == 0 && (e == CHAR_SPACE || e == WORD_SPACE))
  {
    // Don't add leading silence.
  }
  else
  {
    memory[memory_index] = e;
    memory_index++;
  }
}

void memory_add_silence(unsigned long duration)
{
  int dah_length_ms = event_length_ms(DAH);
  if (duration > (dah_length_ms * 1.5))
  {
    memory_add_event(WORD_SPACE);
  }
  else if (duration > (dah_length_ms * 0.5))
  {
    memory_add_event(CHAR_SPACE);
  }
}

