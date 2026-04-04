DEVICE irrigator;
DEVICE light;

LOG "Morning schedule initialized";
SCHEDULE AT 06:30 DO
  irrigator ON;
  LOG "Irrigation cycle started";
END

SCHEDULE AT 19:00 DO
  light ON;
  LOG "Evening lighting enabled";
END
