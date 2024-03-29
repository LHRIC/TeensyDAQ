/*
 *  This file is generated with Embedded Proto, PLEASE DO NOT EDIT!
 *  source: SensorData.proto
 */

// This file is generated. Please do not edit!
#ifndef SENSORDATA_H
#define SENSORDATA_H

#include <cstdint>
#include <MessageInterface.h>
#include <WireFormatter.h>
#include <Fields.h>
#include <MessageSizeCalculator.h>
#include <ReadBufferSection.h>
#include <RepeatedFieldFixedSize.h>
#include <FieldStringBytes.h>
#include <Errors.h>
#include <Defines.h>
#include <limits>

// Include external proto definitions

namespace sensor_data {

template<
    uint32_t SensorMessage_data_LENGTH
>
class SensorMessage final: public ::EmbeddedProto::MessageInterface
{
  public:
    SensorMessage() = default;
    SensorMessage(const SensorMessage& rhs )
    {
      set_timestamp(rhs.get_timestamp());
      set_canId(rhs.get_canId());
      set_data(rhs.get_data());
    }

    SensorMessage(const SensorMessage&& rhs ) noexcept
    {
      set_timestamp(rhs.get_timestamp());
      set_canId(rhs.get_canId());
      set_data(rhs.get_data());
    }

    ~SensorMessage() override = default;

    enum class FieldNumber : uint32_t
    {
      NOT_SET = 0,
      TIMESTAMP = 1,
      CANID = 2,
      DATA = 3
    };

    SensorMessage& operator=(const SensorMessage& rhs)
    {
      set_timestamp(rhs.get_timestamp());
      set_canId(rhs.get_canId());
      set_data(rhs.get_data());
      return *this;
    }

    SensorMessage& operator=(const SensorMessage&& rhs) noexcept
    {
      set_timestamp(rhs.get_timestamp());
      set_canId(rhs.get_canId());
      set_data(rhs.get_data());
      return *this;
    }

    static constexpr char const* TIMESTAMP_NAME = "timestamp";
    inline void clear_timestamp() { timestamp_.clear(); }
    inline void set_timestamp(const uint64_t& value) { timestamp_ = value; }
    inline void set_timestamp(const uint64_t&& value) { timestamp_ = value; }
    inline uint64_t& mutable_timestamp() { return timestamp_.get(); }
    inline const uint64_t& get_timestamp() const { return timestamp_.get(); }
    inline uint64_t timestamp() const { return timestamp_.get(); }

    static constexpr char const* CANID_NAME = "canId";
    inline void clear_canId() { canId_.clear(); }
    inline void set_canId(const uint32_t& value) { canId_ = value; }
    inline void set_canId(const uint32_t&& value) { canId_ = value; }
    inline uint32_t& mutable_canId() { return canId_.get(); }
    inline const uint32_t& get_canId() const { return canId_.get(); }
    inline uint32_t canId() const { return canId_.get(); }

    static constexpr char const* DATA_NAME = "data";
    inline void clear_data() { data_.clear(); }
    inline ::EmbeddedProto::FieldBytes<SensorMessage_data_LENGTH>& mutable_data() { return data_; }
    inline void set_data(const ::EmbeddedProto::FieldBytes<SensorMessage_data_LENGTH>& rhs) { data_.set(rhs); }
    inline const ::EmbeddedProto::FieldBytes<SensorMessage_data_LENGTH>& get_data() const { return data_; }
    inline const uint8_t* data() const { return data_.get_const(); }


    ::EmbeddedProto::Error serialize(::EmbeddedProto::WriteBufferInterface& buffer) const override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;

      if((0U != timestamp_.get()) && (::EmbeddedProto::Error::NO_ERRORS == return_value))
      {
        return_value = timestamp_.serialize_with_id(static_cast<uint32_t>(FieldNumber::TIMESTAMP), buffer, false);
      }

      if((0U != canId_.get()) && (::EmbeddedProto::Error::NO_ERRORS == return_value))
      {
        return_value = canId_.serialize_with_id(static_cast<uint32_t>(FieldNumber::CANID), buffer, false);
      }

      if(::EmbeddedProto::Error::NO_ERRORS == return_value)
      {
        return_value = data_.serialize_with_id(static_cast<uint32_t>(FieldNumber::DATA), buffer, false);
      }

      return return_value;
    };

    ::EmbeddedProto::Error deserialize(::EmbeddedProto::ReadBufferInterface& buffer) override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;
      ::EmbeddedProto::WireFormatter::WireType wire_type = ::EmbeddedProto::WireFormatter::WireType::VARINT;
      uint32_t id_number = 0;
      FieldNumber id_tag = FieldNumber::NOT_SET;

      ::EmbeddedProto::Error tag_value = ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id_number);
      while((::EmbeddedProto::Error::NO_ERRORS == return_value) && (::EmbeddedProto::Error::NO_ERRORS == tag_value))
      {
        id_tag = static_cast<FieldNumber>(id_number);
        switch(id_tag)
        {
          case FieldNumber::TIMESTAMP:
            return_value = timestamp_.deserialize_check_type(buffer, wire_type);
            break;

          case FieldNumber::CANID:
            return_value = canId_.deserialize_check_type(buffer, wire_type);
            break;

          case FieldNumber::DATA:
            return_value = data_.deserialize_check_type(buffer, wire_type);
            break;

          case FieldNumber::NOT_SET:
            return_value = ::EmbeddedProto::Error::INVALID_FIELD_ID;
            break;

          default:
            return_value = skip_unknown_field(buffer, wire_type);
            break;
        }

        if(::EmbeddedProto::Error::NO_ERRORS == return_value)
        {
          // Read the next tag.
          tag_value = ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id_number);
        }
      }

      // When an error was detect while reading the tag but no other errors where found, set it in the return value.
      if((::EmbeddedProto::Error::NO_ERRORS == return_value)
         && (::EmbeddedProto::Error::NO_ERRORS != tag_value)
         && (::EmbeddedProto::Error::END_OF_BUFFER != tag_value)) // The end of the buffer is not an array in this case.
      {
        return_value = tag_value;
      }

      return return_value;
    };

    void clear() override
    {
      clear_timestamp();
      clear_canId();
      clear_data();

    }

    static char const* field_number_to_name(const FieldNumber fieldNumber)
    {
      char const* name = nullptr;
      switch(fieldNumber)
      {
        case FieldNumber::TIMESTAMP:
          name = TIMESTAMP_NAME;
          break;
        case FieldNumber::CANID:
          name = CANID_NAME;
          break;
        case FieldNumber::DATA:
          name = DATA_NAME;
          break;
        default:
          name = "Invalid FieldNumber";
          break;
      }
      return name;
    }

#ifdef MSG_TO_STRING

    ::EmbeddedProto::string_view to_string(::EmbeddedProto::string_view& str) const
    {
      return this->to_string(str, 0, nullptr, true);
    }

    ::EmbeddedProto::string_view to_string(::EmbeddedProto::string_view& str, const uint32_t indent_level, char const* name, const bool first_field) const override
    {
      ::EmbeddedProto::string_view left_chars = str;
      int32_t n_chars_used = 0;

      if(!first_field)
      {
        // Add a comma behind the previous field.
        n_chars_used = snprintf(left_chars.data, left_chars.size, ",\n");
        if(0 < n_chars_used)
        {
          // Update the character pointer and characters left in the array.
          left_chars.data += n_chars_used;
          left_chars.size -= n_chars_used;
        }
      }

      if(nullptr != name)
      {
        if( 0 == indent_level)
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "\"%s\": {\n", name);
        }
        else
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s\"%s\": {\n", indent_level, " ", name);
        }
      }
      else
      {
        if( 0 == indent_level)
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "{\n");
        }
        else
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s{\n", indent_level, " ");
        }
      }
      
      if(0 < n_chars_used)
      {
        left_chars.data += n_chars_used;
        left_chars.size -= n_chars_used;
      }

      left_chars = timestamp_.to_string(left_chars, indent_level + 2, TIMESTAMP_NAME, true);
      left_chars = canId_.to_string(left_chars, indent_level + 2, CANID_NAME, false);
      left_chars = data_.to_string(left_chars, indent_level + 2, DATA_NAME, false);
  
      if( 0 == indent_level) 
      {
        n_chars_used = snprintf(left_chars.data, left_chars.size, "\n}");
      }
      else 
      {
        n_chars_used = snprintf(left_chars.data, left_chars.size, "\n%*s}", indent_level, " ");
      }

      if(0 < n_chars_used)
      {
        left_chars.data += n_chars_used;
        left_chars.size -= n_chars_used;
      }

      return left_chars;
    }

#endif // End of MSG_TO_STRING

  private:


      EmbeddedProto::uint64 timestamp_ = 0U;
      EmbeddedProto::uint32 canId_ = 0U;
      ::EmbeddedProto::FieldBytes<SensorMessage_data_LENGTH> data_;

};

} // End of namespace sensor_data
#endif // SENSORDATA_H