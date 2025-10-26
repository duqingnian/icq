#include "stdafx.h"

#include "Theme.h"
#include "StyleVariable.h"

#include "../fonts.h"
#include "../utils/JsonUtils.h"

namespace
{
    template<class Map>
    auto findProperty(const Map& _map, const QString& _value)
    {
        return std::find_if(_map.begin(), _map.end(), [&_value](const auto& _prop)
        {
            return (_prop.second == _value);
        });
    }

    QColor invalidColor()
    {
        static const QColor clr("#ffee00");
        return clr;
    }

    template <class T>
    void unserializeWallpaperId(const rapidjson::Value& _node, const T& _name, QString& _out)
    {
        if (!JsonUtils::unserialize_value(_node, _name, _out))
        {
            if (auto intId = -1; JsonUtils::unserialize_value(_node, _name, intId))
                _out = QString::number(intId);
        }
        //im_assert(!_out.isEmpty());
    }
}

namespace Styling
{
    void Property::unserialize(const rapidjson::Value& _node)
    {
        if (_node.IsObject())
            JsonUtils::unserialize_value(_node, "color", color_);
        else if (_node.IsString())
            color_ = JsonUtils::getColor(_node);
    }

    QColor Property::getColor() const
    {
        if (color_.isValid())
            return color_;

        return invalidColor();
    }

    Style::Properties::const_iterator Style::getPropertyIt(const StyleVariable _variable) const
    {
        return properties_.find(_variable);
    }

    void Style::unserialize(const rapidjson::Value& _node)
    {
        const auto& props = getVariableStrings();

        for (const auto& it : _node.GetObject())
        {
            const auto propName = JsonUtils::getString(it.name);
            if (const auto property = findProperty(props, propName); property != props.end())
            {
                Property tmpProperty;
                tmpProperty.unserialize(it.value);
                setProperty(stringToVariable(propName), std::move(tmpProperty));
            }
        }
    }

    void Style::setProperty(const StyleVariable _var, Property _property)
    {
        properties_[_var] = std::move(_property);
    }

    QColor Style::getColor(const StyleVariable _variable) const
    {
        if (const auto propIterator = getPropertyIt(_variable); propIterator != properties_.end())
            return propIterator->second.getColor();

        //im_assert(!"Cannot find style color");
        return invalidColor();
    }

    Theme::Theme()
        : id_(qsl("default"))
        , name_(qsl("Noname"))
        , idHash_(qHash(id_))
        , underlinedLinks_(false)
        , isDark_(false)
        , isHidden_(false)
    {
    }

    void Theme::addWallpaper(WallpaperPtr _wallpaper)
    {
        if (!getWallpaper(_wallpaper->getId()))
        {
            if (_wallpaper->getId() == defaultWallpaperId_)
                availableWallpapers_.insert(availableWallpapers_.begin(), _wallpaper);
            else
                availableWallpapers_.push_back(_wallpaper);
        }
    }

    WallpaperPtr Theme::getWallpaper(const WallpaperId& _id) const
    {
        const auto it = std::find_if(availableWallpapers_.begin(), availableWallpapers_.end(), [&_id](const auto& _w) { return _id == _w->getId(); });
        if (it != availableWallpapers_.end())
            return *it;

        return nullptr;
    }

    void Theme::unserialize(const rapidjson::Value& _node)
    {
        if (const auto nodeIter = _node.FindMember("style"); nodeIter->value.IsObject() && !nodeIter->value.ObjectEmpty())
            Style::unserialize(nodeIter->value);

        JsonUtils::unserialize_value(_node, "id", id_);
        JsonUtils::unserialize_value(_node, "name", name_);
        JsonUtils::unserialize_value(_node, "underline", underlinedLinks_);
        JsonUtils::unserialize_value(_node, "dark", isDark_);
        JsonUtils::unserialize_value(_node, "hidden", isHidden_);
        QString id;
        unserializeWallpaperId(_node, "defaultWallpaper", id);
        defaultWallpaperId_.setId(id);

        idHash_ = qHash(id_);
    }

    bool Theme::isWallpaperAvailable(const WallpaperId& _wallpaperId) const
    {
        return std::any_of(
            availableWallpapers_.begin(),
            availableWallpapers_.end(),
                    [&_wallpaperId](const auto& _wall) { return _wall->getId() == _wallpaperId; });
    }

    WallpaperPtr Theme::getFirstWallpaper() const
    {
        if (!availableWallpapers_.empty())
            return availableWallpapers_.front();

        return nullptr;
    }

    ThemeWallpaper::ThemeWallpaper(const Theme& _basicStyle)
    {
        setProperties(_basicStyle.getProperties());
    }

    void ThemeWallpaper::unserialize(const rapidjson::Value& _node)
    {
        //im_assert(_node.HasMember("style"));
        if (const auto nodeIter = _node.FindMember("style"); nodeIter->value.IsObject() && !nodeIter->value.ObjectEmpty())
            Style::unserialize(nodeIter->value);

        QString id;
        unserializeWallpaperId(_node, "id", id);
        id_.setId(id);

        JsonUtils::unserialize_value(_node, "need_border", needBorder_);
        JsonUtils::unserialize_value(_node, "preview", previewUrl_);
        JsonUtils::unserialize_value(_node, "image", imageUrl_);
        JsonUtils::unserialize_value(_node, "color", color_);

        if (hasWallpaper())
        {
            JsonUtils::unserialize_value(_node, "tile", tile_);
            JsonUtils::unserialize_value(_node, "tint", tint_);
        }
    }

    bool ThemeWallpaper::operator==(const ThemeWallpaper & _other)
    {
        const bool equalId = (id_ == _other.id_);
        const bool equalTile = (tile_ == _other.tile_);
        const bool equalBorder = (needBorder_ == _other.needBorder_);
        const bool equalPixmap = (previewUrl_ == _other.previewUrl_ && imageUrl_ == _other.imageUrl_);

        return (equalId && equalTile && equalBorder && equalPixmap);
    }

    void ThemeWallpaper::setWallpaperImage(const QPixmap& _image) // must be tinted beforehand!
    {
        im_assert(!_image.isNull());

        setImageRequested(false);
        image_ = _image;
    }

    void ThemeWallpaper::setPreviewImage(const QPixmap& _image)
    {
        im_assert(!_image.isNull());
        setPreviewRequested(false);
        preview_ = _image;
        Utils::check_pixel_ratio(preview_);
    }

    void ThemeWallpaper::resetWallpaperImage()
    {
        setImageRequested(false);
        image_ = QPixmap();
    }
}
