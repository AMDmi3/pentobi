//-----------------------------------------------------------------------------
/** @file pentobi_kde_thumbnailer/PentobiThumbCreator.h */
//-----------------------------------------------------------------------------

#ifndef PENTOBI_KDE_THUMBNAILER_PENTOBI_THUMB_CREATOR_H
#define PENTOBI_KDE_THUMBNAILER_PENTOBI_THUMB_CREATOR_H

#include <QObject>
#include <kio/thumbcreator.h>

//-----------------------------------------------------------------------------

class PentobiThumbCreator
    : public QObject,
      public ThumbCreator
{
    Q_OBJECT

public:
    virtual ~PentobiThumbCreator();

    bool create(const QString& path, int width, int height,
                QImage& image) override;
};

//-----------------------------------------------------------------------------

#endif // PENTOBI_KDE_THUMBNAILER_PENTOBI_THUMB_CREATOR_H
