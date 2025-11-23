#ifndef _$$CLASS_NAME_UPPER$$_H_
#define _$$CLASS_NAME_UPPER$$_H_
#if $$IFDEF$$

#include "$$BASE_CLASS$$.hpp"
#include "$$BASE_NAME$$.hpp"

class $$CLASS_NAME$$ : public $$BASE_CLASS$$
{
public:
    $$CLASS_NAME$$();
    $$CLASS_NAME$$(uint8_t* buffer);
    $$CLASS_NAME$$(BaseMessage& message);
    $$CLASS_NAME$$($$BASE_NAME$$& body);
    $$CLASS_NAME$$( const $$CLASS_NAME$$ &c ) = default;
    $$CLASS_NAME$$& operator=( const $$CLASS_NAME$$ &c ) = default;
    ~$$CLASS_NAME$$();

    uint32_t SerializeBody() override;
    void DeserializeBody() override;

    $$BASE_NAME$$& Body();


private:
    void ResetBody() override;
    void Init() override;

    $$BASE_NAME$$ mBody;
};

#endif
#endif
