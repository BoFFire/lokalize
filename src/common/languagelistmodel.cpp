/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "languagelistmodel.h"
#include <kglobal.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kicon.h>
#include <klocale.h>

#include <QCoreApplication>
#include <QSortFilterProxyModel>



LanguageListModel* LanguageListModel::_instance=0;
void LanguageListModel::cleanupLanguageListModel()
{
    delete LanguageListModel::_instance; LanguageListModel::_instance = 0;
}

LanguageListModel* LanguageListModel::instance()
{
    if (_instance==0 )
    {
        _instance=new LanguageListModel();
        qAddPostRoutine(LanguageListModel::cleanupLanguageListModel);
    }
    return _instance;
}

LanguageListModel::LanguageListModel(QObject* parent)
 : QStringListModel(KGlobal::locale()->allLanguagesList(),parent)
 , m_sortModel(new QSortFilterProxyModel(this))
{
    KIconLoader::global()->addExtraDesktopThemes();
    //kWarning()<<KIconLoader::global()->hasContext(KIconLoader::International);
    kDebug()<<KIconLoader::global()->queryIconsByContext(KIconLoader::NoGroup,KIconLoader::International);
    //kWarning()<<KGlobal::locale()->allLanguagesList();
    kDebug()<<QLocale("uk").name();
    m_sortModel->setSourceModel(this);
    m_sortModel->sort(0);
}

QVariant LanguageListModel::data(const QModelIndex& index, int role) const
{
    if (role==Qt::DecorationRole)
    {
        QString code=stringList().at(index.row());
        code=QLocale(code).name();
        if (code.contains('_')) code=code.mid(3).toLower();
        return QIcon(KStandardDirs::locate("locale", QString("l10n/%1/flag.png").arg(code)));
    }
    else if (role==Qt::DisplayRole)
    {
        const QString& code=stringList().at(index.row());
        return KGlobal::locale()->languageCodeToName(code)+" ("+code+')';
    }
    return QStringListModel::data(index, role);
}

QFlags< Qt::ItemFlag > LanguageListModel::flags(const QModelIndex& index) const
{
    return QStringListModel::flags(index);
}

int LanguageListModel::sortModelRowForLangCode(const QString& langCode)
{
    return m_sortModel->mapFromSource(index(stringList().indexOf(langCode))).row();
}

QString LanguageListModel::langCodeForSortModelRow(int row)
{
    return stringList().at(m_sortModel->mapToSource(m_sortModel->index(row,0)).row());
}
