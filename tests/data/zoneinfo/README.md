# Time zone files for the tests

`Europe/Paris` and `America/New_York` are compiled files (TZif, version 2) of
the tz database, release 2026f, as `zic` writes them (the "fat" form: the
transitions until 2037, then the POSIX TZ rule of the footer). `test_datetime`
reads them with `Rtimezone_set_directory`, so that its results do not depend
on the zone or the database of the machine.

The tz database is in the public domain: "This file is in the public domain,
so clarified as of 2009-05-17 by Arthur David Olson." (https://www.iana.org/time-zones)
