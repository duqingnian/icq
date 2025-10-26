#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{
    class PhoneInfo
    {
    public:
        std::string info_operator_;
        std::string info_phone_;
        std::string info_iso_country_;
        std::vector<QString> printable_;
        std::string status_;
        std::string trunk_code_;
        std::string modified_phone_number_;
        std::vector<int32_t> remaining_lengths_;
        std::vector<QString> prefix_state_;
        std::string modified_prefix_;
        bool is_phone_valid_;

    public:
        PhoneInfo();
        ~PhoneInfo();
        void deserialize(core::coll_helper *c);

        inline bool isValid() const { return is_phone_valid_; }
    };
}
