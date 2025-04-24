#pragma once
#include "NetSerializeBuffer.h"
#include "NetClient.h"

class {{ client_name }} : public NetClient
{
public:
    virtual void OnConnect();
    virtual void OnRelease();
    virtual void OnRecvMsg(NetSerializeBuffer * msg);
    virtual void OnError(const int8 log_level, int32 err_code, const WCHAR * cause);
    virtual void OnExit();

public:
    {%- for function in parser %}
    {%- if function.type == "CLIENT" %}
    void Marshal_{{ function.name }}({{ function.parameters | join(', ') }});
    {%- endif %}
    {%- endfor %}
    
    {%- for function in parser %}
    {%- if function.type == "SERVER" %}
    void UnMarshal_{{ function.name }}(NetSerializeBuffer * msg);
    {%- endif %}
    {%- endfor %}
    
    {%- for function in parser %}
    {%- if function.type == "SERVER" %}
    void Handle_{{ function.name }}({{ function.parameters | join(', ') }});
    {%- endif %}
    {%- endfor %}

};