# moo is a simple planner/dairy

A dead simple command line planner/diary used to record events that will occur on a specific
date.

There is no functionality to handle times.

The code currently relies on GNU extensions, such as `scandir` and
`getdelim`. I do not have a need to find alternatives to these, however I
am happy to accept pull requests that do.

## Usage

    usage: moo [year] [month] [day] [add|delete] [title] [description] [line]
               [-dmy day/month/year] [-td delim] [-fd delim] [-p path] [-i days] [-o days]

      -td    title-description delimiter
      -fd    file name delimiter
      -p     path to read/write, will contain a moo subfolder
      -i     number of days to include, can be negative
      -o     offset today's date by a number of days, can be negative
    --nosort do not sort the files

If no date is given, the current date is used. A full date (year, month and
day) must be given to add or delete events. Parts of the date can be set using
-d, -m, or -y.

A 'moo' subdirectory in `$XDG_DATA_HOME` will be used to store events if it is
present, otherwise `$HOME/.local/share` will be used. This can be changed with
the `-p` flag. Each date has its own file, named `YYYY-MM-DD` where `-` is the
file name delimiter One event is stored per line in the format
`:title:description`, where `:` is the title-description delimiter.

If a partial date is given, all files starting with the information given will
be listed.

If a title is given but not a description, the description will be blank.

## Searching

The `-i` flag allows you to set a range of days to include in the results. For
example, to display all events in the next 30 days or the past 7 days you would
use `-i 30` and `-i -7` respectively.

You can combine this with the `-o` flag. For example, to find events 5 days
either side of today you would use `-o -5 -i 10`.

You can also enter just the year or just the year and the month to display all
results matching that pattern. `moo 2015 3` would display all events in April
2015.

## Example

Add event for today:

    $ moo add "event title" "event description"

Display the events today

    $ moo 2022-01-03 1 event title event description

Delete an event today

    moo delete 1

Add an event for 3 days time

    moo -o 3 add "tomorrow's event"

Show events on a specific date

    moo 2020 5 12

Show events in the next week

    $ moo week # or $ moo -i 7
