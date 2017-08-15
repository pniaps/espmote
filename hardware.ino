void configureOutputs()
{
  for (byte x = 0; x < 3; x++) {
    if (Settings.cho[x] != 0) {
      int pin = abs(Settings.cho[x]);
      int active = Settings.cho[x] < 0 ? LOW : HIGH;

      //by default, all outputs are disabled
      pinMode(pin, OUTPUT);
      digitalWrite(pin, !active);
    }
  }
}

boolean isOutputDisabled(int channel)
{
  return Settings.cho[channel] == 0;
}

boolean getOutputStatus(int channel)
{
  int pin = abs(Settings.cho[channel]);
  int active = Settings.cho[channel] < 0 ? LOW : HIGH;

  return pin != 0 && digitalRead(pin) == active;
}

boolean setOutputStatus(int channel, boolean activate)
{
  int pin = abs(Settings.cho[channel]);
  int active = Settings.cho[channel] < 0 ? LOW : HIGH;
  if (activate) {
    digitalWrite(pin, active);
  } else {
    digitalWrite(pin, !active);
  }
  return activate;
}

