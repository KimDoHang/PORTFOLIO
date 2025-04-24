#include "pch.h"
#include "{{ client_name }}.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "{{ packet_name }}.h"


void {{ client_name }}::OnConnect()
{
}

void {{ client_name }}::OnRelease()
{
}

void {{ client_name }}::OnRecvMsg(NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		{%- for function in parser %}
		    {%- if function.type == "SERVER" %}
		case en_PACKET_SC_{{ function.name | upper }}:
			UnMarshal_{{ function.name }}(msg);
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
}

void {{ client_name }}::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
}

void {{ client_name }}::OnExit()
{

}

{%- for function in parser %}
     {%- if function.type == "CLIENT" %}
void {{ client_name }}::Marshal_{{ function.name }}({{ function.parameters | join(', ') }})
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_CS_{{ function.name | upper }};

		{%- for type, name, size, digit in function.update_params%}
		    {%- if type == "WCHAR" %}
	msg->PutData(reinterpret_cast<char*>({{ name }}), {{ size }} * 2);
            {%- elif type == "char" %}
    msg->PutData({{ name }}, {{ size }});
            {%- else %}
    (*msg) << {{ name }};
		    {%- endif %}
		{%- endfor %}

	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}
     {%- endif %}
{%- endfor %}

{%- for function in parser %}
     {%- if function.type == "SERVER" %}
void {{ client_name }}::UnMarshal_{{ function.name }}(NetSerializeBuffer* msg)
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
	Handle_{{ function.name }}({{ function.update_params | map(attribute = 1) | join(', ') }});

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
        {%- if function.type == "SERVER" %}
void {{ client_name }}::Handle_{{ function.name }}({{ function.parameters | join(', ') }})
{
}
        {%- endif %}
    {%- endfor %}