DEVICE siren;
DEVICE led;

LOG "Event-driven security automation";
ON motion == 1 THEN
  siren ON;
  led ON;
  LOG "Motion detected: security mode active";
END
