/*
 * DeviceScribbleReader.cpp
 *
 *  Created on: 26 Sep, 2011
 *      Author: joy
 */

#include <vector>
#include <string>

#include "onyx/data/sketch_document.h"
#include "onyx/data/sketch_io.h"
#include "onyx/data/sketch_stroke.h"

#include "../include/DeviceScribbleReader.h"
#include "../include/GlobalDefines.h"
#include "../include/PAUtil.h"
#include "../include/PageScribble.h"
#include "../include/PAPoint.h"
#include "../include/PARect.h"

using namespace pdfanno;

// implementer of PFunc_DeviceCoorTransformer
static bool tranformDeviceCoorToPDF(const PASize &pageSize, PageScribble &pageScribble);

static bool getDeviceScribblePages(SketchDocument &sketch_document, sketch::Pages &pages);
static bool parseDeviceScribblePages(const sketch::Pages &pages, std::vector<PageScribble> &pageScribbles);
static bool parseDeviceScribblePage(const sketch::PageKey &key, const sketch::SketchPagePtr &page, PageScribble &parsedScribble);

bool DeviceScribbleReader::getDocumentScribbles(std::string docPath, std::vector<PageScribble> &pageScribbles)
{
    pageScribbles.clear();

    QString doc_path = QString::fromLocal8Bit(docPath.c_str());

    std::cout<<"sketch file path will be: "<<doc_path.toStdString()<<std::endl;

    SketchDocument sketch_document;
    if (!sketch_document.open(doc_path)) {
        std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"open sketch data failed: "<<doc_path.toStdString()<<std::endl;
        return false;
    }

    sketch::Pages pages;
    if (!getDeviceScribblePages(sketch_document, pages)) {
        return false;
    }

    if (!parseDeviceScribblePages(pages, pageScribbles)) {
        return false;
    }

    return true;
}

PFunc_DeviceCoorTransformer DeviceScribbleReader::getTransformer()
{
    return (PFunc_DeviceCoorTransformer)tranformDeviceCoorToPDF;
}

static bool getDeviceScribblePages(SketchDocument &sketch_document, sketch::Pages &pages)
{
    SketchIOPtr sketch_io = SketchIO::getIO(sketch_document.path(), false);
    if (sketch_io == 0) {
        std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"open sketch data failed: "<<sketch_document.path().toStdString()<<std::endl;
        return false;
    }

    if (!sketch_document.loadAllPages(sketch_io)) {
        std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"open sketch data failed: "<<sketch_document.path().toStdString()<<std::endl;
        return false;
    }

    pages = sketch_document.pages();
    std::cout<<"pages: "<<pages.size()<<std::endl;

    int i = 0;
    QMapIterator<PageKey, SketchPagePtr> it(pages);
    while (it.hasNext()) {
        it.next();
        std::cout << "loading page " << i << std::endl;
        i++;

        SketchPagePtr page = it.value();

        if (!sketch_io->loadPageData(page)) {
            std::cerr<<"loading page failed"<<std::endl;
        }
    }

    sketch_io.get()->close();
    sketch_io.reset(0);

    return true;
}

static bool parseDeviceScribblePage(const sketch::PageKey &key, const sketch::SketchPagePtr &page, PageScribble &parsedScribble)
{
    bool ok = false;
    int num_page = key.toInt(&ok);
    if (!ok) {
        std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"parse PageKey to decimal failed: "<<key.toStdString()<<std::endl;
        assert(false);
        return false;
    }
    parsedScribble.page_ = num_page;

    Strokes strokes = page.get()->strokes();
    std::cout<<strokes.size()<<" strokes in page"<<std::endl;

    int i = 0;
    for (StrokesIter it = strokes.begin(); it != strokes.end(); it++) {
        std::cout<<"parsing stroke "<<i<<std::endl;
        i++;

        const ZoomFactor stroke_zoom_factor = it->get()->zoom();

        std::vector<PAPoint> pa_points;

        Points points = (*it).get()->points();
        std::cout<<points.size()<<" points in stroke"<<std::endl;

        int j = 0;
        for (PointsIter pit = points.begin(); pit != points.end(); pit++) {
            std::cout<<"parsing point "<<j<<" ("<<pit->x() / stroke_zoom_factor<<", "<<
                    pit->y() / stroke_zoom_factor<<")"<<std::endl;
            j++;

            pa_points.push_back(PAPoint(pit->x() / stroke_zoom_factor, pit->y() / stroke_zoom_factor));
        }

        if (pa_points.size() == 0) {
            std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"0 points in SketchStroke"<<std::endl;
            assert(false);
            continue;
        }

        parsedScribble.strokes_.push_back(PageScribble::Stroke(pa_points));

        const PARect &rect = parsedScribble.strokes_.back().rect_;
        std::cout<<"Rect: "<<rect.ll_.x_<<","<<rect.ll_.y_<<","<<rect.ur_.x_<<","<<rect.ur_.y_<<std::endl;
    }

    return true;
}

static bool parseDeviceScribblePages(const sketch::Pages &pages, std::vector<PageScribble> &pageScribbles)
{
    int i = 0;
    QMapIterator<PageKey, SketchPagePtr> it(pages);
    while (it.hasNext()) {
        it.next();

        std::cout<<"parsing page "<<i<<std::endl;
        i++;

        PageKey key = it.key();
        SketchPagePtr page = it.value();

        if (!page->dataLoaded()) {
            std::cout<<"page not loaded yet"<<std::endl;
            assert(false);
            continue;
        }

        PageScribble to_parse;
        if (!parseDeviceScribblePage(key, page, to_parse)) {
            return false;
        }

        pageScribbles.push_back(to_parse);
    }

    return true;
}

static bool tranformDeviceCoorToPDF(const PASize &pageSize, PageScribble &pageScribble)
{
    const int pdf_page_height = pageSize.height_;

    for (std::vector<PageScribble::Stroke>::iterator it = pageScribble.strokes_.begin();
            it != pageScribble.strokes_.end();
            it++) {
        if (it->rect_.ll_.y_ > pdf_page_height) {
            std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"point's coor (" <<
                    it->rect_.ll_.x_ <<", " << it->rect_.ll_.y_ <<
                    ") out of page range height: "<<pdf_page_height<<std::endl;
            assert(false);
            return false;
            continue;
        }
        it->rect_.ll_.y_ = pdf_page_height - it->rect_.ll_.y_;

        if (it->rect_.ur_.y_ > pdf_page_height) {
            std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"point's coor (" <<
                    it->rect_.ur_.x_ <<", " << it->rect_.ur_.y_ <<
                    ") out of page range height: "<<pdf_page_height<<std::endl;
            assert(false);
            return false;
            continue;
        }
        it->rect_.ur_.y_ = pdf_page_height - it->rect_.ur_.y_;
        PAUtil::internalSwap<double>(&(it->rect_.ll_.y_), &(it->rect_.ur_.y_));

        for (std::vector<PAPoint>::iterator pit = (*it).points_.begin();
                pit != it->points_.end();
                pit++) {
            if (pit->y_ > pdf_page_height) {
                std::cerr<<"["<<__FILE__<<", "<<__func__<<", "<<__LINE__<<"]"<<"point's coor (" <<
                        (*pit).x_ <<", " << (*pit).y_ << ") out of page range height: "<<pdf_page_height<<std::endl;
                assert(false);
                return false;
                continue;
            }
            pit->y_ = pdf_page_height - pit->y_;
        }
    }

    return true;
}
