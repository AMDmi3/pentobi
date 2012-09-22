//-----------------------------------------------------------------------------
/** @file libpentobi_gui/ScoreDisplay.h */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_GUI_SCORE_DISPLAY_H
#define LIBPENTOBI_GUI_SCORE_DISPLAY_H

#include <QWidget>
#include "libpentobi_base/Board.h"

using libpentobi_base::Board;
using libpentobi_base::Color;
using libpentobi_base::ColorMap;
using libpentobi_base::Variant;

//-----------------------------------------------------------------------------

class ScoreDisplay
    : public QWidget
{
    Q_OBJECT

public:
    ScoreDisplay(QWidget* parent = 0);

    void updateScore(const Board& bd);

protected:
    void paintEvent(QPaintEvent* event);

    void resizeEvent(QResizeEvent* event);

private:
    int m_fontSize;

    QFont m_font;

    QFont m_fontUnderlined;

    Variant m_variant;

    ColorMap<bool> m_hasMoves;

    ColorMap<unsigned> m_points;

    ColorMap<unsigned> m_bonus;

    int m_colorDotSize;

    int m_colorDotSpace;

    int m_colorDotWidth;

    int m_twoColorDotWidth;

    QString getScoreText(Color c);

    QString getScoreText2(Color c1, Color c2);

    int getScoreTextWidth(Color c);

    int getScoreTextWidth2(Color c1, Color c2);

    void drawScore(QPainter& painter, Color c, int x);

    void drawScore2(QPainter& painter, Color c1, Color c2, int x);

    int getMaxScoreTextWidth() const;

    int getMaxScoreTextWidth2() const;

    QString getScoreText(unsigned points, unsigned bonus) const;

    int getTextWidth(QString text) const;
};

//-----------------------------------------------------------------------------

#endif // LIBPENTOBI_GUI_SCORE_DISPLAY_H
