#include "IRCPaintMainWindow.h"

#include <QtGui/QApplication>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QRegExp>
#include <QFileInfo>

#include <cmath>

IRCPaintMainWindow::IRCPaintMainWindow() {
    colors[0]  = qRgb(255,255,255);
    colors[1]  = qRgb(0  ,0  ,  0);
    colors[2]  = qRgb(0  ,0  ,127);
    colors[3]  = qRgb(0  ,147,  0);
    colors[4]  = qRgb(255,0  ,  0);
    colors[5]  = qRgb(127,0  ,  0);
    colors[6]  = qRgb(156,0  ,156);
    colors[7]  = qRgb(252,127,  0);
    colors[8]  = qRgb(255,255,  0);
    colors[9]  = qRgb(0  ,252,  0);
    colors[10] = qRgb(0  ,147,147);
    colors[11] = qRgb(0  ,255,255);
    colors[12] = qRgb(0  ,0  ,252);
    colors[13] = qRgb(255,0  ,255);
    colors[14] = qRgb(127,127,127);
    colors[15] = qRgb(210,210,210);
    mwidget =  new MainWidget(this, &colors);
    setCentralWidget(mwidget);
    QApplication::setWindowIcon(QIcon(":/IRCPaint.png"));
}

bool IRCPaintMainWindow::exportToTxt(const QString& fname) {
    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("IRC Paint"), tr("Cannot write file %1:\n%2").arg(file.fileName()).arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QImage background = mwidget->getBGImage();
    QImage foreground = mwidget->getFGImage();
    QList<QList<QChar> > text = mwidget->getText();
    int x = 0;
    int y = 0;
    foreach(QList<QChar> l, text) {
        QRgb bg = qRgb(0,0,0);
        QRgb fg = qRgb(0,0,0);
        bool first = true;
        foreach(QChar c, l) {
            if (c.isNull())
                c = ' ';
            QRgb oldfg = fg;
            fg = foreground.pixel(x,y);
            QRgb oldbg = bg;
            bg = background.pixel(x,y);
            if (bg != oldbg || first) { // background color changes, ^Cf,b (or first character in a line)
                int b = rgbToIrc(bg);
                if (c.isDigit() && b < 10) {
                    out << "" << rgbToIrc(fg) << ",0" << b;
                } else {
                    out << "" << rgbToIrc(fg) << "," << b;
                }
            } else if (fg != oldfg) { // only foreground color changes, ^Cf
                int f = rgbToIrc(fg);
                if (c.isDigit() && f < 10) {
                    out << "0" << f;
                } else {
                    out << "" << f;
                }
            }
            out << c;
            ++x;
            first = false;
        }
        out << "\n";
        x = 0;
        ++y;
    }
    QApplication::restoreOverrideCursor();
    return true;
}

bool IRCPaintMainWindow::importFromTxt(const QString& fname) {
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("IRC Paint"), tr("Cannot open file %1:\n%2").arg(file.fileName()).arg(file.errorString()));
        return false;
    }
    static const QRegExp stripCodes("[]|\\d{1,2}(?:,\\d{1,2})?");
    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QStringList lines;
    int width = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        lines << line;
        width = qMax(width, QString(line).replace(stripCodes, "").length());
    }
    file.close();
    int height = lines.size();
    QImage bg(width, height, QImage::Format_RGB32);
    bg.fill(colors[0]);
    QImage fg(width, height, QImage::Format_RGB32);
    fg.fill(colors[1]);
    QList<QList<QChar> > text;
    QRgb bgCol = colors[0];
    QRgb fgCol = colors[1];
    int x = 0;
    int y = 0;
    foreach (QString line, lines) {
        bgCol = colors[0];
        fgCol = colors[1];
        QList<QChar> tLine;
        for (int i = 0; i < line.length(); ++i) {
            if (line[i] == '') {                                                   // ^C
                i = qMin(i+1, line.length());
                if (line[i].isDigit()) {                                            // ^C#
                    i = qMin(i+1, line.length());
                    if (line[i].isDigit()) {                                        // ^C##
                        i = qMin(i+1, line.length());
                        if (line[i] == ',') {                                       // ^C##,
                            i = qMin(i+1, line.length());
                            if (line[i].isDigit()) {                                // ^C##,#
                                i = qMin(i+1, line.length());
                                if (line[i].isDigit()) {                            // ^C##,##
                                    fgCol = ircToRgb(line.mid(i-4,2).toInt());
                                    bgCol = ircToRgb(line.mid(i-1,2).toInt());
                                    continue;
                                }
                                --i;
                                fgCol = ircToRgb(line.mid(i-3,2).toInt());
                                bgCol = ircToRgb(line.mid(i,1).toInt());
                                continue;
                            }
                            --i;
                            fgCol = ircToRgb(line.mid(i-2,2).toInt());
                            --i;
                            continue;
                        }
                        --i;
                        fgCol = ircToRgb(line.mid(i-1,2).toInt());
                        continue;
                    } else if (line[i] == ',') {                                    // ^C#,
                        i = qMin(i+1, line.length());
                        if (line[i].isDigit()) {                                    // ^C#,#
                            i = qMin(i+1, line.length());
                            if (line[i].isDigit()) {                                // ^C#,##
                                bgCol = ircToRgb(line.mid(i-1,2).toInt());
                                fgCol = ircToRgb(line.mid(i-3,1).toInt());
                                continue;
                            }
                            --i;
                            bgCol = ircToRgb(line.mid(i,1).toInt());
                            fgCol = ircToRgb(line.mid(i-2,1).toInt());
                            continue;
                        }
                        --i;
                        fgCol = ircToRgb(line.mid(i-1,1).toInt());
                        --i;
                        continue;
                    }
                    --i;
                    fgCol = ircToRgb(line.mid(i,1).toInt());
                    continue;
                }
                --i;
                fgCol = ircToRgb(1);
                bgCol = ircToRgb(0);
                continue;
            }
            if (line[i].isPrint() && line[i] != '' && line[i] != '' && line[i] != '' && line[i] != '' && line[i] != '') {
                bg.setPixel(x, y, bgCol);
                fg.setPixel(x, y, fgCol);
                tLine << line[i];
                ++x;
            }
        }
        while (tLine.size() < width) {
            tLine << QChar();
        }
        text << tLine;
        ++y;
        x = 0;
    }
    mwidget->swapAscii(width, height, text, bg, fg);
    QApplication::restoreOverrideCursor();
    return true;
}

bool IRCPaintMainWindow::importFromImg(const QString& fname, int maxWidth, bool smooth) {
    QImage image(fname);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("IRC Paint"), tr("Cannot open file %1").arg(QFileInfo(fname).fileName()));
        return false;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QMap<int, Lab> labColors;
    for (int i = 0; i < 16; ++i)
        labColors[i] = rgbToLab(colors[i]);
    if (image.width() > maxWidth/2) {
        if (smooth) {
            image = image.scaledToWidth(maxWidth/2, Qt::SmoothTransformation);
        } else {
            image = image.scaledToWidth(maxWidth/2, Qt::FastTransformation);
        }
    }
    QImage irc(image.width()*2, image.height(), QImage::Format_RGB32);
    QImage fg = irc;
    fg.fill(colors[1]);
    for (int y = 0; y < image.height(); ++y) {
        int j = 0;
        for (int x = 0; x < image.width(); ++x) {
            QRgb color;
            if (qAlpha(image.pixel(x,y)) != 255) {
                color = colors[0];
            } else {
                color = closestColor(image.pixel(x, y), labColors);
            }
            irc.setPixel(x+j, y, color);
            ++j;
            irc.setPixel(x+j, y, color);
        }
    }
    QList<QList<QChar> > t;
    QList<QChar> tmp;
    for (int i = 0; i < irc.width(); ++i)
        tmp << QChar();
    for (int i = 0; i < irc.height(); ++i)
        t << tmp;
    mwidget->swapAscii(irc.width(), irc.height(), t, irc, irc);
    QApplication::restoreOverrideCursor();
    return true;
}

QRgb IRCPaintMainWindow::closestColor(const QRgb& c, const QMap<int, Lab>& labColors) {
    double shortestDistance;
    int index;
    Lab col = rgbToLab(c);
    for (int i = 0; i < 16; ++i) {
        double distance = std::sqrt(std::pow(col.l - labColors[i].l, 2) + std::pow(col.a - labColors[i].a, 2) + std::pow(col.b - labColors[i].b, 2));
        if (i == 0 || distance < shortestDistance) {
            shortestDistance = distance;
            index = i;
        }
    }
    return colors[index];
}

IRCPaintMainWindow::Lab IRCPaintMainWindow::rgbToLab(const QRgb &c) {
    double r = double(qRed(c))   / 255.0;
    double g = double(qGreen(c)) / 255.0;
    double b = double(qBlue(c))  / 255.0;

#define CONVENIENCE(x) if (x > 0.04045) { \
        x = std::pow(((x+0.055)/1.055), 2.4); \
    } else { \
        x /= 12.92; \
    }
    CONVENIENCE(r)
    CONVENIENCE(g)
    CONVENIENCE(b)
#undef CONVENIENCE
    r *= 100.0;
    g *= 100.0;
    b *= 100.0;

    double x = ((r * 0.4124) + (g * 0.3576) + (b * 0.1805)) / 95.047;
    double y = ((r * 0.2126) + (g * 0.7152) + (b * 0.0722)) / 100.0;
    double z = ((r * 0.0193) + (g * 0.1192) + (b * 0.9505)) / 108.883;

#define CONVENIENCE(x) if (x > 0.008856) { \
        x = std::pow(x, 1.0/3.0); \
    } else { \
        x = (7.787 * x) + (16.0 / 116.0); \
    }
    CONVENIENCE(x)
    CONVENIENCE(y)
    CONVENIENCE(z)
#undef CONVENIENCE

    Lab lab;
    lab.l = (116.0 * y) - 16.0;
    lab.a = 500.0 * (x - y);
    lab.b = 200.0 * (y - z);
    return lab;
}

int IRCPaintMainWindow::rgbToIrc(QRgb c) {
    int i = colors.key(c, -1);
    if (i == -1)
        qCritical("FATAL: colors does NOT contain irc representation of %d", c);
    return i;
}

QRgb IRCPaintMainWindow::ircToRgb(int i) {
    while (i > 15)
        i -= 15;
    return colors[i];
}

void IRCPaintMainWindow::swapIrcColor(int i, QRgb c) {
    while (i > 15)
        i -= 15;
    QRgb oldc = colors[i];
    colors[i] = c;
    mwidget->swapColor(oldc, c);
}
