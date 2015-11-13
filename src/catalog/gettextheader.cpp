/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2014 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#include "gettextheader.h"

#include "project.h"

#include "version.h"
#include "prefs_lokalize.h"
#include "prefs.h"

#include <QInputDialog>
#include <QProcess>
#include <QString>
#include <QStringBuilder>
#include <QMap>
#include <QTextCodec>
#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

#include <kdemacros.h>
#include <klocalizedstring.h>

/**
 * this data was obtained by running GNUPluralForms()
 * on all languages KDE knows of
**/
#define NUM_LANG_WITH_INFO 41
static const char* langsWithPInfo[NUM_LANG_WITH_INFO]={
"ar",
"cs",
"da",
"de",
"el",
"en",
"en_GB",
"en_US",
"eo",
"es",
"et",
"fi",
"fo",
"fr",
"ga",
"he",
"hr",
"hu",
"it",
"ja",
"ko",
"lt",
"lv",
"nb",
"nl",
"nn",
"pl",
"pt",
"pt_BR",
"ro",
"ru",
"sk",
"sl",
"sr",
"sr@latin",
"sv",
"th",
"tr",
"uk",
"vi",
"zh_CN"
// '\0'
};

static const char* pInfo[NUM_LANG_WITH_INFO]={
"nplurals=6; plural=n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5;",
"nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n > 1);",
"nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;",
"nplurals=2; plural=(n != 1);",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=1; plural=0;",
"nplurals=1; plural=0;",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n != 1);",
"nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=2; plural=(n != 1);",
"nplurals=2; plural=(n > 1);",
"nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;",
"nplurals=4; plural=(n%100==1 ? 1 : n%100==2 ? 2 : n%100==3 || n%100==4 ? 3 : 0);",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=2; plural=(n != 1);",
"nplurals=1; plural=0;",
"nplurals=2; plural=(n > 1);",
"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
"nplurals=1; plural=0;",
"nplurals=1; plural=0;"
};


int numberOfPluralFormsFromHeader(const QString& header)
{
    QRegExp rxplural("Plural-Forms:\\s*nplurals=(.);");
    if (rxplural.indexIn(header) == -1)
        return 0;
    bool ok;
    int result=rxplural.cap(1).toShort(&ok);
    return ok?result:0;

}

int numberOfPluralFormsForLangCode(const QString& langCode)
{
    QString expr=GNUPluralForms(langCode);

    QRegExp rxplural("nplurals=(.);");
    if (rxplural.indexIn(expr) == -1)
        return 0;
    bool ok;
    int result=rxplural.cap(1).toShort(&ok);
    return ok?result:0;

}

QString GNUPluralForms(const QString& lang)
{
    QByteArray l(lang.toUtf8());
    int i=NUM_LANG_WITH_INFO;
    while(--i>=0 && l!=langsWithPInfo[i])
        ;
    //if (KDE_ISLIKELY( langsWithPInfo[i]))
    if (KDE_ISLIKELY( i>=0 ))
        return QString::fromLatin1(pInfo[i]);


    //BEGIN alternative
    // NOTE does this work under M$ OS?
    qWarning()<<"gonna call msginit";
    QString def="nplurals=2; plural=n != 1;";

    QStringList arguments;
    arguments << "-l" << lang
              << "-i" << "-"
              << "-o" << "-"
              << "--no-translator"
              << "--no-wrap";
    QProcess msginit;
    msginit.start("msginit", arguments);

    msginit.waitForStarted(5000);
    if (KDE_ISUNLIKELY( msginit.state()!=QProcess::Running ))
    {
        //qWarning()<<"msginit error";
        return def;
    }

    msginit.write(
                   "# SOME DESCRIPTIVE TITLE.\n"
                   "# Copyright (C) YEAR Free Software Foundation, Inc.\n"
                   "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n"
                   "#\n"
                   "#, fuzzy\n"
                   "msgid \"\"\n"
                   "msgstr \"\"\n"
                   "\"Project-Id-Version: PACKAGE VERSION\\n\"\n"
                   "\"POT-Creation-Date: 2002-06-25 03:23+0200\\n\"\n"
                   "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
                   "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
                   "\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n"
                   "\"Language: LL\\n\"\n"
                   "\"MIME-Version: 1.0\\n\"\n"
                   "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
                   "\"Content-Transfer-Encoding: ENCODING\\n\"\n"
//                   "\"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\\n\"\n"
                  );
    msginit.closeWriteChannel();

    if (KDE_ISUNLIKELY( !msginit.waitForFinished(5000) ))
    {
        qWarning()<<"msginit error";
        return def;
    }


    QByteArray result = msginit.readAll();
    int pos = result.indexOf("Plural-Forms: ");
    if (KDE_ISUNLIKELY( pos==-1 ))
    {
        //qWarning()<<"msginit error"<<result;
        return def;
    }
    pos+=14;

    int end = result.indexOf('"',pos);
    if (KDE_ISUNLIKELY( pos==-1 ))
    {
        //qWarning()<<"msginit error"<<result;
        return def;
    }

    return QString( result.mid(pos,end-pos-2) );
    //END alternative
}


void updateHeader(QString& header,
                  QString& comment,
                  QString& langCode,
                  int& numberOfPluralForms,
                  const QString& CatalogProjectId,
                  bool generatedFromDocbook,
                  bool belongsToProject,
                  bool forSaving,
                  QTextCodec* codec)
{
    askAuthorInfoIfEmpty();

    QStringList headerList(header.split('\n',QString::SkipEmptyParts));
    QStringList commentList(comment.split('\n',QString::SkipEmptyParts));

//BEGIN header itself
    QStringList::Iterator it,ait;
    QString temp;
    QString authorNameEmail;

    const QString BACKSLASH_N = QStringLiteral("\\n");

    // Unwrap header since the following code
    // assumes one header item per headerList element
    it = headerList.begin();
    while ( it != headerList.end() )
    {
        if (!(*it).endsWith(BACKSLASH_N))
        {
            const QString line = *it;
            it = headerList.erase(it);
            if (it != headerList.end())
            {
                *it = line + *it;
            }
            else
            {
                // Something bad happened, put a warning on the command line
                qWarning() << "Bad .po header, last header line was" << line;
            }
        }
        else
        {
            ++it;
        }
    }

    bool found=false;
    authorNameEmail=Settings::authorName();
    if (!Settings::authorEmail().isEmpty())
        authorNameEmail+=(QStringLiteral(" <")%Settings::authorEmail()%'>');
    temp=QStringLiteral("Last-Translator: ") % authorNameEmail % BACKSLASH_N;

    QRegExp lt(QStringLiteral("^ *Last-Translator:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
    {
        if (it->contains(lt))
        {
            if (forSaving) *it = temp;
            found=true;
        }
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    QString dateTimeString = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    QString zoneOffsetString1 = QTimeZone(QTimeZone::systemTimeZoneId()).displayName(QTimeZone::GenericTime, QTimeZone::OffsetName);
    int zpos=qMax(qMax(0, zoneOffsetString1.indexOf('+')), zoneOffsetString1.indexOf('-'));
    QString zoneOffsetString = QString::fromRawData(zoneOffsetString1.unicode()+zpos, zoneOffsetString1.length()-zpos);
    temp=QStringLiteral("PO-Revision-Date: ") % dateTimeString % zoneOffsetString.remove(':') % BACKSLASH_N;
    QRegExp poRevDate(QStringLiteral("^ *PO-Revision-Date:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
    {
        found=it->contains(poRevDate);
        if (found && forSaving) *it = temp;
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    temp=QStringLiteral("Project-Id-Version: ") % CatalogProjectId % BACKSLASH_N;
    //temp.replace( "@PACKAGE@", packageName());
    QRegExp projectIdVer(QStringLiteral("^ *Project-Id-Version:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
    {
        found=it->contains(projectIdVer);
        if (found && it->contains(QLatin1String("PACKAGE VERSION")))
            *it = temp;
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    
    langCode=Project::instance()->isLoaded()?
             Project::instance()->langCode():
             Settings::defaultLangCode();
    QString language; //initialized with preexisting value or later
    QString mailingList; //initialized with preexisting value or later

    static QMap<QString,QLocale::Language> langEnums;
    if (!langEnums.size())
    for (int l=QLocale::Abkhazian; l<=QLocale::Akoose; ++l)
        langEnums[QLocale::languageToString((QLocale::Language)l)]=(QLocale::Language)l;
    
    static QRegExp langTeamRegExp(QStringLiteral("^ *Language-Team:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
    {
        found=it->contains(langTeamRegExp);
        if (found)
        {
            //really parse header
            QRegExp re(QStringLiteral("^ *Language-Team: *(.*) *<([^>]*)>"));
            if (re.indexIn(*it) != -1 )
            {
                if (langEnums.contains( re.cap(1).trimmed() ))
                {
                    language=re.cap(1).trimmed();
                    mailingList=re.cap(2).trimmed();
                    QList<QLocale> locales = QLocale::matchingLocales(langEnums.value(language), QLocale::AnyScript, QLocale::AnyCountry);
                    if (locales.size()) langCode=locales.first().name().left(2);
                }
            }

            ait=it;
        }
    }

    if (language.isEmpty())
    {
        language=QLocale::languageToString(QLocale(langCode).language());
        if (language.isEmpty())
            language=langCode;
    }

    if (mailingList.isEmpty() || belongsToProject)
    {
        if (Project::instance()->isLoaded())
            mailingList=Project::instance()->mailingList();
        else //if (mailingList.isEmpty())
            mailingList=Settings::defaultMailingList();
    }



    temp=QStringLiteral("Language-Team: ")%language%QStringLiteral(" <")%mailingList%QStringLiteral(">\\n");
    if (KDE_ISLIKELY( found ))
        (*ait) = temp;
    else
        headerList.append(temp);

    static QRegExp langCodeRegExp(QStringLiteral("^ *Language: *([^ \\\\]*)"));
    temp=QStringLiteral("Language: ") % langCode % BACKSLASH_N;
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
    {
        found=(langCodeRegExp.indexIn(*it)!=-1);
        if (found && langCodeRegExp.cap(1).isEmpty())
            *it=temp;
        //if (found) qWarning()<<"got explicit lang code:"<<langCodeRegExp.cap(1);
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    temp=QStringLiteral("Content-Type: text/plain; charset=") % codec->name() % BACKSLASH_N;
    QRegExp ctRe(QStringLiteral("^ *Content-Type:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
    {
        found=it->contains(ctRe);
        if (found) *it=temp;
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);


    found=false;
    temp=QStringLiteral("Content-Transfer-Encoding: 8bit\\n");
    QRegExp cteRe(QStringLiteral("^ *Content-Transfer-Encoding:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end() && !found; ++it )
        found=it->contains(cteRe);
    if (!found)
        headerList.append(temp);

    // ensure MIME-Version header
    temp=QStringLiteral("MIME-Version: 1.0\\n");
    QRegExp mvRe(QStringLiteral("^ *MIME-Version:"));
    for ( it = headerList.begin(),found=false; it != headerList.end()&& !found; ++it )
    {
        found=it->contains(mvRe);
        if (found) *it = temp;
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);


    //qDebug()<<"testing for GNUPluralForms";
    // update plural form header
    QRegExp pfRe(QStringLiteral("^ *Plural-Forms:"));
    for ( it = headerList.begin(),found=false; it != headerList.end()&& !found; ++it )
        found=it->contains(pfRe);
    if (found)
    {
        --it;

        //qDebug()<<"GNUPluralForms found";
        int num=numberOfPluralFormsFromHeader(header);
        if (!num)
        {
            if (generatedFromDocbook)
                num=1;
            else
            {
                qWarning()<<"No plural form info in header, using project-defined one"<<langCode;
                QString t=GNUPluralForms(langCode);
                //qWarning()<<"generated: " << t;
                if ( !t.isEmpty() )
                {
                    static QRegExp pf(QStringLiteral("^ *Plural-Forms:\\s*nplurals.*\\\\n"));
                    pf.setMinimal(true);
                    temp=QStringLiteral("Plural-Forms: %1\\n").arg(t);
                    it->replace(pf,temp);
                    num=numberOfPluralFormsFromHeader(temp);
                }
                else
                {
                    qWarning()<<"no... smth went wrong :(\ncheck your gettext install";
                    num=2;
                }
            }
        }
        numberOfPluralForms=num;

    }
    else if ( !generatedFromDocbook)
    {
        //qDebug()<<"generating GNUPluralForms"<<langCode;
        QString t = GNUPluralForms(langCode);
        //qDebug()<<"here it is:";
        if ( !t.isEmpty() ) {
            const QString pluralFormLine=QStringLiteral("Plural-Forms: %1\\n").arg(t);
            headerList.append(pluralFormLine);
            numberOfPluralForms=numberOfPluralFormsFromHeader(pluralFormLine);
        }
    }

    temp=QStringLiteral("X-Generator: Lokalize %1\\n");
    temp=temp.arg(LOKALIZE_VERSION);
    QRegExp xgRe(QStringLiteral("^ *X-Generator:.*"));
    for ( it = headerList.begin(),found=false; it != headerList.end()&& !found; ++it )
    {
        found=it->contains(xgRe);
        if (found) *it = temp;
    }
    if (KDE_ISUNLIKELY( !found ))
        headerList.append(temp);

    //m_header.setMsgstr( headerList.join( "\n" ) );
    header=headerList.join(QStringLiteral("\n"));
//END header itself

//BEGIN comment = description, copyrights
    // U+00A9 is the Copyright sign
    QRegExp fsfc(QStringLiteral("^# *Copyright (\\(C\\)|\\x00a9).*Free Software Foundation, Inc"));
    for ( it = commentList.begin(),found=false; it != commentList.end()&&!found; ++it )
    {
        found=it->contains( fsfc ) ;
        if (found)
            it->replace(QStringLiteral("YEAR"), QDate::currentDate().toString(QStringLiteral("yyyy")));
    }
/*
                        	    if( saveOptions.FSFCopyright == ProjectSettingsBase::Update )
                        	    {
                        		    //update years
                        		    QString cy = QDate::currentDate().toString("yyyy");
                        		    if( !it->contains( QRegExp(cy)) ) // is the year already included?
                        		    {
                        			int index = it->lastIndexOf( QRegExp("[\\d]+[\\d\\-, ]*") );
                        			if( index == -1 )
                        			{
                        			    KMessageBox::information(0,i18n("Free Software Foundation Copyright does not contain any year. "
                        			    "It will not be updated."));
                        			} else {
                        			    it->insert(index+1, QString(", ")+cy);
                        			}
                        		    }
                        	    }*/
#if 0
    if ( ( !usePrefs || saveOptions.updateDescription )
            && ( !saveOptions.descriptionString.isEmpty() ) )
    {
        temp = "# "+saveOptions.descriptionString;
        temp.replace( "@PACKAGE@", packageName());
        temp.replace( "@LANGUAGE@", identityOptions.languageName);
        temp = temp.trimmed();

        // The description strings has often buggy variants already in the file, these must be removed
        QString regexpstr = "^#\\s+" + QRegExp::escape( saveOptions.descriptionString.trimmed() ) + "\\s*$";
        regexpstr.replace( "@PACKAGE@", ".*" );
        regexpstr.replace( "@LANGUAGE@", ".*" );
        //qDebug() << "REGEXPSTR: " <<  regexpstr;
        QRegExp regexp ( regexpstr );

        // The buggy variants exist in English too (of a time before KBabel got a translation for the corresponding language)
        QRegExp regexpUntranslated ( "^#\\s+translation of .* to .*\\s*$" );


        qDebug () << "Temp is '" << temp << "'";

        found=false;
        bool foundTemplate=false;

        it = commentList.begin();
        while ( it != commentList.end() )
        {
            qDebug () << "testing '" << (*it) << "'";
            bool deleteItem = false;

            if ( (*it) == temp )
            {
                qDebug () << "Match ";
                if ( found )
                    deleteItem = true;
                else
                    found=true;
            }
            else if ( regexp.indexIn( *it ) >= 0 )
            {
                // We have a similar (translated) string (from another project or another language (perhaps typos)). Remove it.
                deleteItem = true;
            }
            else if ( regexpUntranslated.indexIn( *it ) >= 0 )
            {
                // We have a similar (untranslated) string (from another project or another language (perhaps typos)). Remove it.
                deleteItem = true;
            }
            else if ( (*it) == "# SOME DESCRIPTIVE TITLE." )
            {
                // We have the standard title placeholder, remove it
                deleteItem = true;
            }

            if ( deleteItem )
                it = commentList.erase( it );
            else
                ++it;
        }
        if (!found) commentList.prepend(temp);
    }
#endif
    // qDebug() << "HEADER COMMENT: " << commentList;

    /*    if ( (!usePrefs || saveOptions.updateTranslatorCopyright)
            && ( ! identityOptions->readEntry("authorName","").isEmpty() )
            && ( ! identityOptions->readEntry("Email","").isEmpty() ) ) // An email address can be used as ersatz of a name
        {*/
//                        return;
    QStringList foundAuthors;

    temp=QStringLiteral("# ")%authorNameEmail%QStringLiteral(", ")%QDate::currentDate().toString(QStringLiteral("yyyy"))%'.';

    // ### TODO: it would be nice if the entry could start with "COPYRIGHT" and have the "(C)" symbol (both not mandatory)
    QRegExp regexpAuthorYear( QStringLiteral("^#.*(<.+@.+>)?,\\s*([\\d]+[\\d\\-, ]*|YEAR)") );
    QRegExp regexpYearAlone( QStringLiteral("^# , \\d{4}.?\\s*$") );
    if (commentList.isEmpty())
    {
        commentList.append(temp);
        commentList.append(QString());
    }
    else
    {
        it = commentList.begin();
        while ( it != commentList.end() )
        {
            bool deleteItem = false;
            if ( it->indexOf( QLatin1String("copyright"), 0, Qt::CaseInsensitive ) != -1 )
            {
                // We have a line with a copyright. It should not be moved.
            }
            else if ( it->contains( QRegExp(QStringLiteral("#, *fuzzy")) ) )
                deleteItem = true;
            else if ( it->contains( regexpYearAlone ) )
            {
                // We have found a year number that is preceded by a comma.
                // That is typical of KBabel 1.10 (and earlier?) when there is neither an author name nor an email
                // Remove the entry
                deleteItem = true;
            }
            else if ( it->contains( QLatin1String("# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.")) )
                deleteItem = true;
            else if ( it->contains( QLatin1String("# SOME DESCRIPTIVE TITLE")))
                deleteItem = true;
            else if ( it->contains( regexpAuthorYear ) ) // email address followed by year
            {
                if ( !foundAuthors.contains( (*it) ) )
                {
                    // The author line is new (and not a duplicate), so add it to the author line list
                    foundAuthors.append( (*it) );
                }
                // Delete also non-duplicated entry, as now all what is needed will be processed in foundAuthors
                deleteItem = true;
            }

            if ( deleteItem )
                it = commentList.erase( it );
            else
                ++it;
        }

        if ( !foundAuthors.isEmpty() )
        {
            found = false;
            bool foundAuthor = false;

            const QString cy = QDate::currentDate().toString(QStringLiteral("yyyy"));

            ait = foundAuthors.end();
            for ( it = foundAuthors.begin() ; it!=foundAuthors.end(); ++it )
            {
                if ( it->contains(Settings::authorName()) || it->contains(Settings::authorEmail()) )
                {
                    foundAuthor = true;
                    if ( it->contains( cy ) )
                        found = true;
                    else
                        ait = it;
                }
            }
            if ( !found )
            {
                if ( !foundAuthor )
                    foundAuthors.append(temp);
                else if ( ait != foundAuthors.end() )
                {
                    //update years
                    const int index = (*ait).lastIndexOf( QRegExp(QStringLiteral("[\\d]+[\\d\\-, ]*")) );
                    if ( index == -1 )
                        (*ait)+=QStringLiteral(", ")%cy;
                    else
                        ait->insert(index+1, QStringLiteral(", ")%cy);
                }
                else
                    qDebug() << "INTERNAL ERROR: author found but iterator dangling!";
            }

        }
        else
            foundAuthors.append(temp);


        foreach (QString author, foundAuthors)
        {
            // ensure dot at the end of copyright
            if ( !author.endsWith(QLatin1Char('.')) ) author += QLatin1Char('.');
            commentList.append(author);
        }
    }

    //m_header.setComment( commentList.join( "\n" ) );
    comment=commentList.join(QStringLiteral("\n"));

//END comment = description, copyrights
}



QString fullUserName();// defined in <platform>helpers.cpp

void askAuthorInfoIfEmpty()
{
    if (Settings::authorName().isEmpty())
    {
        bool ok;
        QString contact = QInputDialog::getText(
            SettingsController::instance()->mainWindowPtr(),
            i18nc("@window:title", "Author name missing"), i18n("Your name:"),
            QLineEdit::Normal, fullUserName(), &ok);

#ifndef NOKDE
        Settings::self()->authorNameItem()->setValue(ok?contact:fullUserName());
        Settings::self()->save();
#else
        Settings::self()->setAuthorName(ok?contact:fullUserName());
#endif
    }
    if (Settings::authorEmail().isEmpty())
    {
        bool ok;
        QString email = QInputDialog::getText(
            SettingsController::instance()->mainWindowPtr(),
            i18nc("@window:title", "Author email missing"), i18n("Your email:"),
            QLineEdit::Normal, QString(), &ok);

        if (ok)
        {
#ifndef NOKDE
            Settings::self()->authorEmailItem()->setValue(email);
            Settings::self()->save();
#else
            Settings::self()->setAuthorEmail(email);
#endif
        }
    }
}
