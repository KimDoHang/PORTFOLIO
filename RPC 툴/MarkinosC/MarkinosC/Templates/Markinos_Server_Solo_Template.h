#pragma once
#include "SoloInstance.h"
#include "NetSendPacket.h"
#include "NetSerializeBuffer.h"

class {{ solo_name }} : public SoloInstance
{
public:
    {{ solo_name }}(NetService * service, uint16 logic_id) : SoloInstance(service, logic_id)
    {

    }

    virtual void OnEnter(uint64 session_id, SessionInstance * instance) override;
    virtual void OnLeave(uint64 session_id, SessionInstance * instance) override;
    virtual void OnRelease(uint64 session_id, SessionInstance * instance) override;
    virtual bool OnRecvMsg(uint64 session_id, SessionInstance * instance, NetSerializeBuffer * msg) override;

public:
    {%- for function in parser %}
        {%- if function.type == "SERVER" %}
            {%- if function.update_params %}
    void Marshal_{{ function.name }}(uint64 session_id,{{ function.parameters | join(', ') }});
            {%- else %}
    void Marshal_{{ function.name }}(uint64 session_id);
            {%- endif %}
        {%- endif %}
     {%- endfor %}

	{%- for function in parser %}
        {%- if function.type == "CLIENT" %}
    void UnMarshal_{{ function.name }}(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);
        {%- endif %}
    {%- endfor %}

    {%- for function in parser %}
        {%- if function.type == "CLIENT" %}
            {%- if function.update_params %}
    void Handle_{{ function.name }}(uint64 session_id, SessionInstance* instance, {{ function.parameters | join(', ') }});
            {%- else %}
    void Handle_{{ function.name }}(uint64 session_id, SessionInstance* instance);
            {%- endif %}
        {%- endif %}
    {%- endfor %}
};