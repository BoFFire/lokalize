#ifndef POS_H
#define POS_H


enum Part {UndefPart, Msgid, Msgstr, Comment};

/**
* This struct represents a position in a catalog.
* A position is a tuple (index,pluralform,textoffset).
*
* @short Structure, that represents a position in a catalog.
* @author Matthias Kiefer <matthias.kiefer@gmx.de>
* @author Stanislav Visnovsky <visnovsky@kde.org>
*/
struct DocPosition
{
    Part part:16;
    short form:16;
    int entry:32;
    uint offset:32;

    DocPosition():
        part(Msgstr),
        form(0),
        entry(-1), 
        offset(0)
        {}
};

#endif
