#include "pch.h"
#include "NetService.h"
#include "{{ solo_name }}.h"
#include "{{ packet_name }}.h"

void {{ solo_name }}::OnEnter(uint64 session_id, SessionInstance* instance)
{
}

void {{ solo_name }}::OnLeave(uint64 session_id, SessionInstance* instance)
{
}

void {{ solo_name }}::OnRelease(uint64 session_id, SessionInstance* instance)
{
}

bool {{ solo_name }}::OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		{%- for function in parser %}
		    {%- if function.type == "CLIENT" %}
		case en_PACKET_CS_{{ function.name | upper }}:
			UnMarshal_{{ function.name }}(session_id, instance, msg);
			break;
		    {%- endif %}
		{%- endfor %}
		default:
			break;
		}

	}
	catch (const NetMsgException& net_msg_exception)
	{
	}
	return true;
}

{%- for function in parser %}
     {%- if function.type == "SERVER" %}
		{%- if function.update_params %}
void {{ solo_name }}::Marshal_{{ function.name }}(uint64 session_id, {{ function.parameters | join(', ') }})
        {%- else %}
void {{ solo_name }}::Marshal_{{ function.name }}(uint64 session_id)
        {%- endif %}
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_SC_{{ function.name | upper }};

		{%- for type, name, size, digit in function.update_params%}
		    {%- if type == "WCHAR" %}
	msg->PutData(reinterpret_cast<char*>({{ name }}), {{ size }} * 2);
            {%- elif type == "char" %}
    msg->PutData({{ name }}, {{ size }});
            {%- else %}
    (*msg) << {{ name }};
		    {%- endif %}
		{%- endfor %}

	_service->SendPacket(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}
     {%- endif %}
{%- endfor %}

{%- for function in parser %}
     {%- if function.type == "CLIENT" %}
void {{ solo_name }}::UnMarshal_{{ function.name }}(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{

		{%- for type, name, size, digit in function.update_params%}
		    {%- if type == "WCHAR" %}
				{%- if digit %}	
	WCHAR {{ name }}[{{ size }}];
				{%- endif %}
            {%- elif type == "char" %}
				{%- if digit %}	
    char {{ name }}[{{ size }}]; 
				{%- else %}
	char* {{ name }} = new char[{{ size }}];
				{%- endif %}
            {%- else %}
	{{ type }} {{ name }};
		    {%- endif %}
		{%- endfor %}

	{% if condition %}
    \n
	{% endif %}


		{%- for type, name, size, digit in function.update_params%}
		    {%- if type == "WCHAR" %}
			{%- if digit %}	
				{%- else %}
	WCHAR* {{ name }} = new WCHAR[{{ size }}];
				{%- endif %}
	msg->GetData(reinterpret_cast<char*>({{ name }}), {{ size }} * 2);
            {%- elif type == "char" %}
    msg->GetData({{ name }}, {{ size }});
            {%- else %}
	(*msg) >> {{ name }};
		    {%- endif %}
		{%- endfor %}
	{% if condition %}
		\n
	{% endif %}
        {%- if function.update_params %}
    Handle_{{ function.name }}(session_id, instance, {{ function.update_params | map(attribute = 1) | join(', ') }});
        {% else %}
    Handle_{{ function.name }}(session_id, instance);
        {%- endif %}

		{%- for type, name, size, digit in function.update_params%}
			{%- if type == "WCHAR" %}
				{%- if digit == false %}	
	delete[] {{ name }};
				{%- endif %}
            {%- elif type == "char" %}
				{%- if digit == false %}	
    delete[] {{ name }}; 
				{%- endif %}
		    {%- endif %}
		{%- endfor %}
}
     {%- endif %}
{%- endfor %}


    {%- for function in parser %}
        {%- if function.type == "CLIENT" %}
			{%- if function.update_params %}
void {{ solo_name }}::Handle_{{ function.name }}(uint64 session_id, SessionInstance* instance, {{ function.parameters | join(', ') }})
			{%- else %}
void {{ solo_name }}::Handle_{{ function.name }}(uint64 session_id, SessionInstance* instance)
			{%- endif %}
{
}
        {%- endif %}
    {%- endfor %}