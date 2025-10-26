#include "stdafx.h"

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/enumerations.h"

#include "FileSharingInfo.h"

using namespace core;

namespace HistoryControl
{
    FileSharingInfo::FileSharingInfo(const coll_helper &info)
        : ContentType_(std::make_unique<core::file_sharing_content_type>())
    {
        IsOutgoing_ = info.get_value_as_bool("outgoing");

        UploadingProcessId_ = QString::fromUtf8(info.get_value_as_string("uploading_id", ""));

        if (info.is_value_exist("local_path"))
        {
            im_assert(!info.is_value_exist("uri"));
            im_assert(IsOutgoing_);
            LocalPath_ = QString::fromUtf8(info.get_value_as_string("local_path"));
        }
        else
        {
            Uri_ = QString::fromUtf8(info.get_value_as_string("uri"));
            im_assert(!Uri_.isEmpty());
        }

        if (!info.is_value_exist("content_type"))
        {
            if (info.is_value_exist("base_content_type"))
                ContentType_->type_ = file_sharing_base_content_type(info.get_value_as_int("base_content_type"));
            if (info.is_value_exist("duration"))
                duration_ = info.get_value_as_int64("duration");
            return;
        }

        ContentType_->type_ = (file_sharing_base_content_type) info.get_value_as_int("content_type");
        ContentType_->subtype_ = (file_sharing_sub_content_type)info.get_value_as_int("content_subtype");

        if (ContentType_->type_ == file_sharing_base_content_type::ptt)
            return;

        const auto width = info.get_value_as_int("width");
        const auto height = info.get_value_as_int("height");
        im_assert(width > 0);
        im_assert(height > 0);
        Size_ = std::make_unique<QSize>(width, height);
    }

    const QString& FileSharingInfo::GetUri() const
    {
        return Uri_;
    }

    const QString& FileSharingInfo::GetLocalPath() const
    {
        im_assert(IsOutgoing());

        return LocalPath_;
    }

    QSize FileSharingInfo::GetSize() const
    {
        return Size_ ? *Size_ : QSize();
    }

    bool FileSharingInfo::IsOutgoing() const
    {
        return IsOutgoing_;
    }

    const QString& FileSharingInfo::GetUploadingProcessId() const
    {
        im_assert(IsOutgoing());

        return UploadingProcessId_;
    }

    bool FileSharingInfo::HasLocalPath() const
    {
        return !LocalPath_.isEmpty();
    }

    bool FileSharingInfo::HasSize() const
    {
        return !!Size_;
    }

    bool FileSharingInfo::HasUri() const
    {
        return !Uri_.isEmpty();
    }

    void FileSharingInfo::SetUri(const QString &uri)
    {
        im_assert(IsOutgoing());
        Uri_ = uri;
    }

    QString FileSharingInfo::ToLogString() const
    {
        return u"\toutgoing=<"
        % ql1s(logutils::yn(IsOutgoing_))
        % u">\n"
        % u"\tlocal_path=<"
        % LocalPath_
        % u">\n"
        % u"\tuploading_id=<"
        % UploadingProcessId_
        % u">\n"
        % u"\turi=<"
        % Uri_
        % u'>';
    }

    core::file_sharing_content_type& FileSharingInfo::getContentType() const
    {
        return *ContentType_;
    }

    void FileSharingInfo::setContentType(const core::file_sharing_content_type& type)
    {
        *ContentType_ = type;
    }

    const std::optional<qint64>& FileSharingInfo::duration() const noexcept
    {
        return duration_;
    }
}