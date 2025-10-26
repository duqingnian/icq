#include "stdafx.h"

#include "../../corelib/collection_helper.h"

#include "../utils/utils.h"

#include "LoadPixmapFromDataTask.h"

namespace Utils
{
    LoadPixmapFromDataTask::LoadPixmapFromDataTask(core::istream *stream, const QSize& _maxSize)
        : Stream_(stream)
        , maxSize_(_maxSize)
    {
        im_assert(Stream_);

        Stream_->addref();
    }

    LoadPixmapFromDataTask::~LoadPixmapFromDataTask()
    {
        Stream_->release();
    }

    void LoadPixmapFromDataTask::run()
    {
        const auto size = Stream_->size();
        im_assert(size > 0);

        auto data = QByteArray::fromRawData((const char *)Stream_->read(size), (int)size);
        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return;
        QPixmap preview;
        QSize originalSize;
        Utils::loadPixmapScaled(data, maxSize_, Out preview, Out originalSize);

        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return;

        if (preview.isNull())
        {
            Q_EMIT loadedSignal(QPixmap());
            return;
        }

        Q_EMIT loadedSignal(preview);
    }

    LoadImageFromDataTask::LoadImageFromDataTask(core::istream* _stream, const QSize& _maxSize)
        : stream_(_stream),
          maxSize_(_maxSize)
    {
        im_assert(stream_);

        stream_->addref();
    }

    LoadImageFromDataTask::~LoadImageFromDataTask()
    {
        stream_->release();
    }

    void LoadImageFromDataTask::run()
    {
        const auto size = stream_->size();
        im_assert(size > 0);

        auto data = QByteArray::fromRawData((const char *)stream_->read(size), (int)size);

        QImage image;
        QSize originalSize;
        Utils::loadImageScaled(data, maxSize_, Out image, Out originalSize);

        if (!image.isNull())
            Q_EMIT loaded(image);
        else
            Q_EMIT loadingFailed();
    }

}
