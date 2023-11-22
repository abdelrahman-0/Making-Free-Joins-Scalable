import os
import json
import sys
sys.path.append('..')
from mappings import MAPPINGS


src = 'original'
dest = 'preprocessed'
schema = os.path.join('..', 'schema', 'job.dbschema.json')

SCHEMA = {}
ASSIGNED = []
FJ_ATTR_IDX = {}
GLOB_FJ_IDX = 0


def load_schema(path: str):
    global SCHEMA
    with open(path, 'r') as f:
        SCHEMA = json.load(f)


def get_table(binding: str):
    return MAPPINGS[''.join(filter(lambda x: x.isalpha(), binding))]


def process_plan(plan: dict):
    if plan['name'] not in ['HASH_JOIN', 'SEQ_SCAN', 'PROJECTION']:
        return process_plan(plan['children'][0])
    if plan['name'] == 'PROJECTION':
        if '.' not in plan['extra_info']:
            return process_plan(plan['children'][0])
        else:
            result = {}
            result['name'] = 'projection'
            attributes = []
            for bind_attr_str in plan['extra_info'].split('\n'):
                if not bind_attr_str:
                    continue
                binding, attribute = bind_attr_str.split('.')
                bind_attr_dict = {
                    'binding': binding,
                    'attribute': attribute,
                    'table': get_table(binding)
                }
                attributes.append(bind_attr_dict)
            result['attributes'] = attributes
            result['children'] = [process_plan(plan['children'][0])]
    elif plan['name'] == 'HASH_JOIN':
        result = {}
        result['name'] = 'hashjoin'
        assert plan['extra_info'].startswith('INNER\n'), "Bad join (e.g: mark join, ...)"
        conditions = []
        for cond_str in plan['extra_info'][6:].split('\n'):
            if not cond_str:
                continue
            conds_left_right = cond_str.replace(' ', '').replace('\n', '').split('=')
            cond_dict = {}
            for label, cond in zip(['left', 'right'], conds_left_right):
                binding, attribute = cond.split('.')
                cond_dict[label] = {'binding': binding, 'attribute': attribute, 'table': get_table(binding)}
            conditions.append(cond_dict)
        result['conditions'] = conditions
        result['children'] = [process_plan(plan['children'][0]), process_plan(plan['children'][1])]
    elif plan['name'] == 'SEQ_SCAN':
        result = {}
        result['name'] = 'scan'
        result['table'] = plan['extra_info'].split('\n')[0]
    return result


def assign_bindings_to_scans(plan: dict):
    # check all possibilities
    if plan['name'] == 'projection':
        plan['children'][0] = assign_bindings_to_scans(plan['children'][0])
        return plan
    elif plan['name'] == 'hashjoin':
        left_is_scan = plan['children'][0]['name'] == 'scan'
        right_is_scan = plan['children'][1]['name'] == 'scan'
        if left_is_scan and right_is_scan:
            # both children are scans -> assign bindings to both
            left_cond = plan['conditions'][0]['left']
            right_cond = plan['conditions'][0]['right']
            if plan['children'][0]['table'] == left_cond['table']:
                plan['children'][0]['binding'] = left_cond['binding']
                plan['children'][1]['binding'] = right_cond['binding']
            elif plan['children'][0]['table'] == right_cond['table']:
                plan['children'][0]['binding'] = right_cond['binding']
                plan['children'][1]['binding'] = left_cond['binding']
            ASSIGNED.append(left_cond['binding'])
            ASSIGNED.append(right_cond['binding'])
        elif not (left_is_scan or right_is_scan):
            # both children are joins -> skip current node
            for child in [0, 1]:
                plan['children'][child] = assign_bindings_to_scans(plan['children'][child])
        else:
            # exactly one child is a table scan -> assign bindings to scan child
            if left_is_scan:
                singleton = 0
            elif right_is_scan:
                singleton = 1
            plan['children'][1 - singleton] = assign_bindings_to_scans(plan['children'][1 - singleton])
            left_cond = plan['conditions'][0]['left']
            right_cond = plan['conditions'][0]['right']
            if left_cond['binding'] not in ASSIGNED:
                plan['children'][singleton]['binding'] = left_cond['binding']
                ASSIGNED.append(left_cond['binding'])
            elif right_cond['binding'] not in ASSIGNED:
                plan['children'][singleton]['binding'] = right_cond['binding']
                ASSIGNED.append(right_cond['binding'])
        return plan
    else:
        assert False, "Unreachable"


def assign_fj_idxs(plan: dict):
    # Note that this only works because the JOB benchmark queries include the full
    # transitive closure for equijoin predicates
    if plan['name'] == 'scan':
        return
    elif plan['name'] == 'projection':
        assign_fj_idxs(plan['children'][0])
        return
    elif plan['name'] == 'hashjoin':
        assign_fj_idxs(plan['children'][0])
        assign_fj_idxs(plan['children'][1])
        for cond in plan['conditions']:
            left_key = (cond['left']['binding'],cond['left']['attribute'])
            right_key = (cond['right']['binding'],cond['right']['attribute'])
            left_assigned = left_key in FJ_ATTR_IDX.keys()
            right_assigned = right_key in FJ_ATTR_IDX.keys()
            if not left_assigned and not right_assigned:
                global GLOB_FJ_IDX
                FJ_ATTR_IDX[left_key] = GLOB_FJ_IDX
                FJ_ATTR_IDX[right_key] = GLOB_FJ_IDX
                GLOB_FJ_IDX += 1
            elif left_assigned and not right_assigned:
                FJ_ATTR_IDX[right_key] = FJ_ATTR_IDX[left_key]
            elif not left_assigned and right_assigned:
                FJ_ATTR_IDX[left_key] = FJ_ATTR_IDX[right_key]
            elif FJ_ATTR_IDX[left_key] < FJ_ATTR_IDX[right_key]:
                FJ_ATTR_IDX[right_key] = FJ_ATTR_IDX[left_key]
            elif FJ_ATTR_IDX[left_key] > FJ_ATTR_IDX[right_key]:
                FJ_ATTR_IDX[left_key] = FJ_ATTR_IDX[right_key]
            else:
                continue
        return
    return


def update_node_idxs(plan: dict):
    if plan['name'] == 'scan':
        for attr in plan['required_attributes']:
            _key = (plan['binding'],attr['attribute'])
            if _key in FJ_ATTR_IDX:
                attr['fj_attribute_idx'] = FJ_ATTR_IDX[_key]
            else:
                # projection attributes have no free join index
                attr['fj_attribute_idx'] = -1
        return plan
    elif plan['name'] == 'projection':
        plan['children'][0] = update_node_idxs(plan['children'][0])
        return plan
    elif plan['name'] == 'hashjoin':
        plan['children'][0] = update_node_idxs(plan['children'][0])
        plan['children'][1] = update_node_idxs(plan['children'][1])
        for cond in plan['conditions']:
            left_key = (cond['left']['binding'],cond['left']['attribute'])
            right_key = (cond['right']['binding'],cond['right']['attribute'])
            assert FJ_ATTR_IDX[left_key] == FJ_ATTR_IDX[right_key], "Mismatching FJ attribute indexes"
            cond['left']['fj_attribute_idx'] = FJ_ATTR_IDX[left_key]
            cond['right']['fj_attribute_idx'] = FJ_ATTR_IDX[right_key]
        return plan


def offer_bindings(plan: dict):
    if plan['name'] == 'scan':
        return {'singleton': plan['binding']}, [plan['binding']]
    elif plan['name'] == 'projection':
        singleton_dict, bindings = offer_bindings(plan['children'][0])
        return {'singleton': bindings, 'children': [singleton_dict]}, bindings
    elif plan['name'] == 'hashjoin':
        left_dict, left_bindings = offer_bindings(plan['children'][0])
        right_dict, right_bindings = offer_bindings(plan['children'][1])
        combined_dict = {
            'left': left_bindings,
            'right': right_bindings,
            'children': [left_dict, right_dict]
            }
        return combined_dict, left_bindings + right_bindings
    else:
        assert False, "Unreachable"


def request_attributes(plan: dict, helper_plan: dict, needed_attributes: list):
    if plan['name'] == 'scan':
        # required attributes could either be projections or join attributes
        required_attributes = []
        table_schema = [t['columns'] for t in SCHEMA['tables'] if t['name'] == needed_attributes[0]['table']]
        assert len(table_schema) == 1, "Duplicate table names"
        table_schema = table_schema[0]
        for attr in needed_attributes:
            attr['table_attribute_idx'] = [i for i in range(len(table_schema)) if table_schema[i]['name'] == attr['attribute']][0]
            # _key = (plan['binding'],plan['attribute'])
            # attr['fj_attribute_idx'] = FJ_ATTR_IDX[_key]
            required_attributes.append(attr)
        plan['required_attributes'] = required_attributes
        return plan
    elif plan['name'] == 'projection':
        request = plan['attributes']
        plan['children'][0] = request_attributes(plan['children'][0], helper_plan['children'][0], request)

        for attr in plan['attributes']:
            table_schema = [t['columns'] for t in SCHEMA['tables'] if t['name'] == attr['table']][0]
            attr['table_attribute_idx'] = [i for i in range(len(table_schema)) if table_schema[i]['name'] == attr['attribute']][0]

        return plan
    elif plan['name'] == 'hashjoin':
        left_request = []
        right_request = []
        # TODO remove duplicates
        for attr in needed_attributes:
            if attr['binding'] in helper_plan['left'] and attr not in left_request:
                left_request.append(attr.copy())
            if attr['binding'] in helper_plan['right'] and attr not in right_request:
                right_request.append(attr.copy())

        for cond in plan['conditions']:
            for child in ['left', 'right']:
                if cond[child]['binding'] in helper_plan['left'] and cond[child] not in left_request:
                    left_request.append(cond[child].copy())
                if cond[child]['binding'] in helper_plan['right'] and cond[child] not in right_request:
                    right_request.append(cond[child].copy())
        
        plan['children'][0] = request_attributes(plan['children'][0], helper_plan['children'][0], left_request)
        plan['children'][1] = request_attributes(plan['children'][1], helper_plan['children'][1], right_request)

        for cond in plan['conditions']:
            for child in ['left', 'right']:
                table_schema = [t['columns'] for t in SCHEMA['tables'] if t['name'] == cond[child]['table']][0]
                cond[child]['table_attribute_idx'] = [i for i in range(len(table_schema)) if table_schema[i]['name'] == cond[child]['attribute']][0]
        return plan
    else:
        assert False, "Unreachable"


def fix_scan_attributes(plan: dict):
    helper_plan = offer_bindings(preprocessed)[0]
    plan = request_attributes(plan, helper_plan, [])
    return plan
    

if __name__ == '__main__':
    load_schema(schema)
    for query in os.listdir(src):
        # if '6c.json' != query:
        #     continue
        ASSIGNED = []
        FJ_ATTR_IDX = {}
        GLOB_FJ_IDX = 0
        with open(os.path.join(src, query), 'r') as f:
            original = json.load(f)
        try:
            preprocessed = process_plan(original)

        except AssertionError as e:
            print(f'Could not parse {query} due to the following assertion error: ', end='')
            print(e, end='')
            print(', continuing...')
            continue
        processed = assign_bindings_to_scans(preprocessed)
        assign_fj_idxs(processed)
        processed = fix_scan_attributes(processed)
        processed = update_node_idxs(processed)
        with open(os.path.join(dest, query), 'w') as f:
            json.dump(processed, f, indent=2)
        # print(json.dumps(processed, indent=2))
