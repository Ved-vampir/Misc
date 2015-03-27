import sys

from copy import deepcopy

import matplotlib.pyplot as plt

import io_test

key_pos = {'blocksize': 0, 'direct_io': 1, 'action': 2, 'concurence': 3}
actions = ['randwrite', 'randread', 'read', 'write']

def get_key(x, no):
    """ x = (), no = key_pos key """
    keys = deepcopy(key_pos)
    del keys[no]
    key = [x[n] for n in keys.values()]
    return tuple(key), x[key_pos[no]]

def generate_groups(data, group_id):
    """ select data for plot by group_id
        data - processed_series"""
    grouped = {}
    
    for key, val in data.items():
        new_key, group_val = get_key(key, group_id)
        group = grouped.setdefault(new_key, {})
        group[group_val] = val

    return grouped

def save_plot(key, val):
    """ one plot from one dict item with value list"""
    oy = []
    ox = []
    for y in sorted(val.keys()):
        oy.append(y)
        ox.append(val[y][0])
    name = "_".join(str(k) for k in key)
    plt.plot(ox, oy, label=name)
    #plt.legend ((pl), (name))
    #plt.savefig(name+'.png', format='png', dpi=100)
    #print oy, ">>", ox
    #plt.show()
    #plt.clf()

def plot_generation(fname, group_by):
    """ plots for value group_by in imgs by actions"""
    CONC_POS = key_pos['concurence']
    data = list(io_test.load_io_py_file(fname))
    item = io_test.Data("hdr")
    for key, vals in io_test.groupby_globally(data, io_test.key_func).items():
        item.series[key] = [val['iops'] * key[CONC_POS] for val in vals]
    io_test.process_inplace(item)

    pr_data = generate_groups(item.processed_series, group_by)

    for action in actions:
        for key, val in pr_data.items():
            if action in key:
                save_plot(key, val)
        plt.legend()
        plt.title("Plot for %s on %s" % (group_by, action))
        plt.xlabel("IOPS")
        plt.ylabel(group_by)
        plt.axis([0.0, 5000.0, 0.0, 64.0])
        name = "%s__%s.png" % (group_by, action)
        plt.savefig(name, format='png', dpi=100)
        plt.clf()


def deviation_on_deviation(groups_list, data):
    """ calc deviation of data all and by selection groups"""
    total_dev = io_test.round_deviation(io_test.med_dev(data))
    grouped_dev = [total_dev]
    for group in groups_list:
        beg = 0
        end = group
        local_dev = []
        while end <= len(data):
            local_dev.append(io_test.round_deviation(io_test.med_dev(data[beg:end]))[0])
            beg += group
            end += group
        grouped_dev.append(io_test.round_deviation(io_test.med_dev(local_dev)))
    return grouped_dev



def deviation_generation(fname, groups_list):
    CONC_POS = key_pos['concurence']
    int_list = [int(i) for i in groups_list]
    data = list(io_test.load_io_py_file(fname))
    item = io_test.Data("hdr")
    for key, vals in io_test.groupby_globally(data, io_test.key_func).items():
        item.series[key] = [val['iops'] * key[CONC_POS] for val in vals]
        print deviation_on_deviation(int_list, item.series[key])

def main(argv):
    if argv[1] == "plot":
        plot_generation(argv[2], argv[3])
    elif argv[1] == "dev":
        deviation_generation(argv[2], argv[3:])
    

if __name__ == "__main__":
    exit(main(sys.argv))




