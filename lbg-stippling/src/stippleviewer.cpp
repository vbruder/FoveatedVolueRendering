#include "stippleviewer.h"

#include <cassert>
#include <iostream>

#include <QCoreApplication>
#include <QPrinter>
#include <QSvgGenerator>


StippleViewer::StippleViewer(const QImage& img, QWidget* parent)
    : QGraphicsView(parent), m_image(img) {
    setInteractive(false);
    setRenderHint(QPainter::Antialiasing, true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setFrameStyle(0);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setCacheMode(QGraphicsView::CacheBackground);

    setScene(new QGraphicsScene(this));
    this->scene()->setSceneRect(m_image.rect());
    this->scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    this->scene()->addPixmap(QPixmap::fromImage(m_image));

    m_stippling = LBGStippling();
    m_stippling.setStatusCallback([this](const auto& status) {
        this->scene()->clear();
        emit iterationStatus(status.iteration + 1, status.size, status.splits, status.merges,
                             status.hysteresis);
    });
    m_stippling.setCellCallback([this](const auto& cells) { displayCells(cells); });
    m_stippling.setStippleCallback([this](const auto& stipples) { displayPoints(stipples); });
}

void StippleViewer::saveImage()
{
    QString path = QString::number(m_count++) + ".png";
    QPixmap map = this->getImage();
    map.save(path);
}

void StippleViewer::displayPoints(const std::vector<Stipple>& stipples) {
    for (const auto& s : stipples) {
        auto x = s.pos.x() * m_image.width() - s.size / 2.0;
        auto y = s.pos.y() * m_image.height() - s.size / 2.0;
        this->scene()->addEllipse(x, y, s.size, s.size, Qt::NoPen, s.color);
    }
    // TODO: Fix event handling
    QCoreApplication::processEvents();
    saveImage();
}

void StippleViewer::displayCells(const IndexMap &map)
{
    QImage image(map.width, map.height, QImage::Format_RGB32);
    for (size_t y = 0; y < map.height; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (size_t x = 0; x < map.width; ++x) {
            size_t index = map.get(x, y);
            line[x] = index;
        }
    }
    this->scene()->addPixmap(QPixmap::fromImage(image));
    // TODO: Fix event handling
    QCoreApplication::processEvents();
}

QPixmap StippleViewer::getImage() {
    QPixmap pixmap(this->scene()->sceneRect().size().toSize());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    this->scene()->render(&painter);
    return pixmap;
}

void StippleViewer::saveImageSVG(const QString& path) {
    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(QSize(this->scene()->width(), this->scene()->height()));
    generator.setViewBox(this->scene()->sceneRect());
    generator.setTitle("Stippling Result");
    generator.setDescription("SVG File created by Weighted Linde-Buzo-Gray Stippling");
    QPainter painter;
    painter.begin(&generator);
    this->scene()->render(&painter);
    painter.end();
}

void StippleViewer::saveImagePDF(const QString& path) {
    adjustSize();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setCreator("Weighted Linde-Buzo-Gray Stippling");
    printer.setOutputFileName(path);
    printer.setPaperSize(m_image.size(), QPrinter::Point);
    printer.setFullPage(true);

    QPainter painter(&printer);
    this->render(&painter);
}

void StippleViewer::setInputImage(const QImage& img) {
    m_image = img;
    this->scene()->clear();
    this->scene()->addPixmap(QPixmap::fromImage(m_image));
    this->scene()->setSceneRect(m_image.rect());

    auto w = m_image.width();
    auto h = m_image.height();

    setMinimumSize(w, h);
    adjustSize();

    emit inputImageChanged();
}

void StippleViewer::stipple(const LBGStippling::Params params) {
    // TODO: Handle return value
    m_stippling.stipple(m_image, params);
    finished();
}
