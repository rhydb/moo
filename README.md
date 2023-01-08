# moo is a simple planner/dairy

A dead simple planner/diary used to record events that will occur on a specific date.

It does not manage deadlines or times, but there is nothing stoppping you from entering those yourself.

## Usage

    usage: moo [year] [month] [day] [add|delete] [title] [description]
        [-d delim] [-fd delim] [-p path]

        -d    title-description delimiter
        -fd   file name delimiter
        -p    path to read/write, will contain a moo subfolder

If no date is given, the current date is used. A full date (year, month and day) must be given to add or delete events.

A 'moo' subdirectory in `$XDG_DATA_HOME` will be used to store events if it is present, otherwise `$HOME/.local/share` will be used. This can be changed with the `-p` flag.
Each date has its own file, named `YYYY-MM-DD` where `-` is the file name delimiter
One event is stored per line in the format `:title:description`, where `:` is the title-description delimiter.

If a partial date is given, all files starting with the information given will be listed.

If a title is given but not a description, the description will be blank.

## Example

    $ moo 2022 1 2 add "an event" "with a description"
    $ moo 2022 1 3 add "another event"
    $ moo 2022
    2022-01-03
        1 another event
    2022-01-02
        1 an event
            with a description
