import re
from collections import defaultdict

class MarkinosParser:
    def __init__(self):
        self.text = []
        self.groups = defaultdict(lambda: {"functions": []})
        self.solos = defaultdict(lambda: {"functions": []})
        self.proxy_list = []
    

    def parse_midl(self, filePath):
        file = open(filePath, 'r', encoding='cp949')
        self.text = file.read()
        self.parse()


    def parse(self):
        # Group과 Solo 블록을 찾기 위한 정규식
        block_pattern = re.finditer(r'#(Group|Solo)\s*=\s*"([^"]+)"\s*\{([\s\S]*?)\}', self.text)

        start_num = 0

        for match in block_pattern:
            block_type, block_name, content = match.groups()
            parsed_rpc, start_num = self.extract_functions(content, start_num, only_rpc=True)

            if block_type == "Group":
                self.groups[block_name]["functions"].extend(parsed_rpc)
            elif block_type == "Solo":
                self.solos[block_name]["functions"].extend(parsed_rpc)

        # PROXY 공통 리스트 추출 (모든 RPC 및 PROXY 포함)
        start_num = 0 
        self.proxy_list, start_num = self.extract_functions(self.text, start_num, only_rpc=False)

    def extract_functions(self, content, start_num, only_rpc=False):
      
        function_pattern = re.finditer(r'(CLIENT|SERVER)\s*:\s*([a-zA-Z_][a-zA-Z0-9_]*)\((.*?)\)\s*(~\s*[0-9]+)?', content)
        parsed_functions = []

        for match in function_pattern:
            type_, name, params, num = match.groups()
            param_list = self.parse_params(params)

            if num is None:
                num = start_num
                start_num = start_num + 1
            else:
                num = int(num.strip('~'))
                start_num = num + 1

            
            parameters = []
            update_params = []

            for param_type, param_val in param_list:
                 if '[' in param_val and ']' in param_val:
                     param_name, param_size = param_val.split('[')
                     param_size = param_size.split(']')[0]
                     parameters.append(param_type + '* ' + param_name)
                     update_params.append([param_type, param_name, param_size, param_size.isdigit()])
                 else:
                     parameters.append(param_type + ' ' + param_val)
                     update_params.append([param_type, param_val, None, None])
            
            
            function_data = {
                "type": type_,
                "name": name,
                "num" : num,
                "parameters" : parameters,
                "update_params" : update_params
            }

            # 그룹 내부에서는 RPC만 저장, proxy_list는 RPC + PROXY 저장
            parsed_functions.append(function_data)

        return parsed_functions, start_num


    def parse_params(self, params):
        param_pattern = re.findall(r'([\w\*\s]+?)\s+([\w_]+(?:\[[^\]]+\])?)(?:,|$)', params)
        return [[p_type.strip(), p_name.strip()] for p_type, p_name in param_pattern]

    def get_result(self):
        return {
            "groups": self.groups,  # Group 내부 RPC 함수 저장
            "solos": self.solos,    # Solo 내부 RPC 함수 저장
            "proxy": self.proxy_list  # 전체 RPC 및 PROXY 저장
        }