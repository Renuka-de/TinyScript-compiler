DEVICE fan;
DEVICE led;

LOG "Thermostat controller starting";

IF temperature > 30 THEN
  fan ON;
  led ON;
ELSE
  fan OFF;
  led OFF;
END