#include "UInt256Buffer.hpp"
UInt256Buffer::~UInt256Buffer() {
    UInt256Buffer::Reset();
}

UInt256Buffer::UInt256Buffer() {
    UInt256Buffer::Reset();
}

uint32_t UInt256Buffer::Serialize(void* toBuffer) const {
    memcpy(toBuffer, this->data(), MaxSerializedLength);
    return MaxSerializedLength;
}

uint32_t UInt256Buffer::Deserialize(const void* fromBuffer) {
    memcpy(this->data(), fromBuffer, MaxSerializedLength);
    return MaxSerializedLength;
}

size_t UInt256Buffer::TotalLength() const { return MaxSerializedLength; }

void UInt256Buffer::Reset() {
    this->fill(0);
}

void UInt256Buffer::Set(const uint8_t* fromBuffer) {
    Deserialize(fromBuffer);
}

void UInt256Buffer::Set(const uint8_t* fromBuffer, size_t length) {
    Reset();
    if(length > MaxSerializedLength)
        return;
    memcpy(this->data() + (MaxSerializedLength - length), fromBuffer, length);
}

bool UInt256Buffer::IsZero() {
    uint8_t zeros[MaxSerializedLength] = {0};
    return 0 == memcmp(zeros, this->data(), MaxSerializedLength);
}

bool UInt256Buffer::operator==(const UInt256Buffer& rhs) {
    return static_cast<std::array<uint8_t, MaxSerializedLength>>(*this) == static_cast<std::array<uint8_t, MaxSerializedLength>>(rhs);
}
bool UInt256Buffer::operator!=(const UInt256Buffer& rhs) {
    return !(*(this) == rhs);
}
