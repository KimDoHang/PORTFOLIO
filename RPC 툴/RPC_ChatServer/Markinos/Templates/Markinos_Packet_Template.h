#pragma once



enum MARKINOS_PACKET_HEADER : uint16
{
		{%- for function in parser %}
		    {%- if function.type == "CLIENT" %}
	en_PACKET_CS_{{ function.name | upper }} = {{ function.num }},
		    {%- else %}
	en_PACKET_SC_{{ function.name | upper }} = {{ function.num }},
		    {%- endif %}
		{%- endfor %}
		
};