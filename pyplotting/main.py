import numpy as np
import matplotlib.pyplot as plt

def print_hi(name):
    # Use a breakpoint in the code line below to debug your script.
    print(f'Hi, {name}')  # Press Ctrl+F8 to toggle the breakpoint.

def readfile(infile):
    # Parses a benchmark log: keeps only lines whose first token is a key=value
    # pair (the per-run result lines emitted by the C++ harness).
    unsorted_testruns = []
    with open(infile, 'r') as file1:
        for line in file1:
            tokenized = line.split(" ")
            if "=" in tokenized[0]:
                unsorted_testruns.append(tokenized)

    return unsorted_testruns

def filter_list(unsort, token):
    filtered = []
    for elem in unsort:
        for subelem in elem:
            if subelem == token:
                filtered.append(elem)
    return filtered

# this is highly individual, adjust depending on how your measured stuff looks like
def combine_matching(matching_runs):
    print("here are matching runs: {}".format(matching_runs))



def combine_nine_into_one(uncombined, token):

    combined_runs = []
    for e in range(1, 10):
        changetoken = token + str(e)
        # print("these are the es::{}, finding with changetoken::{}".format(e, changetoken))

        matching_runs = []
        # we assume that everything is in the same order
        for elem in uncombined:
            if changetoken in elem:
                # print("Changetoken {} links up with: \n {}".format(changetoken, elem))
                matching_runs.append(elem)
        combined_runs.append(combine_matching(matching_runs))

def combine_similar(uncombined, constraint_list):
    # for idx, val in enumerate(ints): << maybe that is useful
    combined = []
    unique_constraint_map = {}
    for pos, run in enumerate(uncombined):
        # print("run{} , pos {}".format(run, pos))
        run_constraints = []
        for constraint in constraint_list: # find unique constraints if list elem.
            for subelem in run:
                if constraint in subelem:
                    run_constraints.append(subelem)

        rc_str = "".join(str(item) for item in run_constraints)  # makes a unique string out of constraints
        if rc_str not in unique_constraint_map:  # only progress if we have not encountered this combination
            # print("unique_str: {}".format(rc_str))

            unique_constraint_map[rc_str] = 1
            same_runs = [run]
            # now go over rest of the list, adding these matching runs together
            for elem in uncombined:
                if elem != run:
                    found = 0
                    for constraint in run_constraints:
                        # print("rc: {}".format(run_constraints))
                        for subelem in elem:
                            if subelem == constraint:
                                found += 1
                        # print("found={}".format(found))
                    if found == len(run_constraints):
                        # print("matchy: {}".format(elem))
                        same_runs.append(elem)

            # print("same_run-size should be 9!={}".format(len(same_runs)))
            if len(same_runs) != 9:
                print("LISTITEMS DO NOT ADD UP TO NINE, but to {}, check if thats an error".format(len(same_runs)))
                # exit(1)

            combined.append(same_runs)

    print("combined-len{}".format(len(combined))) # 72 as wanted || depends of dataset of course
    return combined

# assumed is that all have the same categories
def squash_nine(to_squash, excluded_fields):
    category_dict = {}
    # each group in squash should become 1
    squashed_groups = []
    for elem in to_squash[0][0]:  # first get all categories, we assume that all are same-shaped
        # print("hello elem {}".format(elem))
        tokens = elem.split("=")
        category = tokens[0]
        if category not in category_dict and category not in excluded_fields:
            # print("adding cat {}".format(category))
            category_dict[category] = 1

    for group in to_squash:
        # go over cats,
        # sum them together, divide by nine, find min/max
        # print("cat-dict should be full: {}".format(category_dict))
        squash = group[0]  # init the squash with all values
        for cat in category_dict:
            cat_min = float('inf')
            cat_max = -float('inf')
            cat_sum = 0
            # print("taking cat{}".format(cat))
            for run in group:
                for value in run:
                    tokenized = value.split("=")
                    if cat == tokenized[0]:
                        testval_str = tokenized[1]
                        # if cat == "qv0.9":
                        #     print(" here is a testval:{}".format(testval_str))
                        testval = float(testval_str)
                        # print("testval:{}".format(type(testval)))
                        cat_sum += testval
                        if testval < cat_min:
                            cat_min = testval
                        if testval > cat_max:
                            cat_max = testval
            # print("taking cat{}".format(cat))
            # add averaged_values to squash now
            res_min = cat + "_min=" + str(cat_min)
            res_max = cat + "_max=" + str(cat_max)
            # print("here is the len{}, catsum".format(len(group)))
            res_avg = cat + "_avg=" + str(cat_sum / len(group))
            cat_avg  = cat_sum / len(group)
            # if cat_avg < cat_min or cat_avg > cat_max:
                # print("cats are deviated, avg{}, sum{}, grouplen{}".format(cat_avg, cat_sum, len(group)))
            squash.append(res_min)
            squash.append(res_max)
            squash.append(res_avg)
        # print("squash done? {}".format(squash))
        squashed_groups.append(squash)
        # print("squashed_groups should now be:{}".format(len(squashed_groups)))
    print("squashed_groups should now be xd:{}".format(len(squashed_groups)))
    print("squash[0] should now be xd:{}".format(squashed_groups[0]))
    return squashed_groups

# good for finding x-series
def find_series_in_squash(series, token):
    res_series = []
    print("token{}".format(token))
    for elem in series:
        for subelem in elem:
            tokenized = subelem.split("=")
            if token == tokenized[0]:
                res_series.append(tokenized[1])

    # sort it in case its unsorted
    placeholder = np.array(res_series)  # TODO cleanup make function
    res_series = placeholder.astype(float)
    res_series.sort()
    return res_series

def filter_yseries_in_squash(x_series, x_token, squashes, token, constraints):
    selected_squashes = []
    if constraints[0] == "no":
        selected_squashes = squashes
    else:
        for elem in squashes:  # select the relevant squashes
            for subelem in elem:
                tokenized = subelem.split("=")
                for con in constraints:
                    t_con = con.split("=")
                    if subelem == con and len(subelem) == len(con):
                        selected_squashes.append(elem)

    y_min = []
    y_avg = []
    y_max = []
    #arrange y-vals according to x
    for x in x_series:
        # go through selected_squashes, check that the x matches, then get the associated  value
        # to avoid casting wrong x'es
        x_str = ""
        if x < 1:
            x_str = str(x)
        else:
            x_str = str(int(x))


        right_squash = False
        for sq in selected_squashes:
            for subelem in sq:  # checks if that squash is the right val
                tokenized = subelem.split("=")
                if tokenized[0] == x_token and x_str == tokenized[1]:  # example: x_token is 'size'
                    right_squash = True
                    break
            if right_squash:  # find and add the right vals to the y-series
                tkn_min = token + "min"
                tkn_max = token + "max"
                tkn_avg = token + "avg"
                for item in sq:
                    tokenized = item.split("=")
                    if tkn_min == tokenized[0]:
                        y_min.append(tokenized[1])
                    if tkn_avg == tokenized[0]:
                        y_avg.append(tokenized[1])
                    if tkn_max == tokenized[0]:
                        y_max.append(tokenized[1])
            right_squash = False

    y_minn = np.array(y_min)  # TODO cleanup make function
    y_min = y_minn.astype(float)

    y_avgn = np.array(y_avg)
    y_avg = y_avgn.astype(float)

    y_maxn = np.array(y_max)
    y_max = y_maxn.astype(float)
    return y_avg - y_min, y_avg, y_max - y_avg

def err_bar_dd_eq_input(dd_squashed, eq_squashed):
    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=2"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=2')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=4"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=4')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=8"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=8')

    # eq done
    y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_add_s_", ["no"])
    yerr_eq = [y_eq_max, y_eq_min]
    plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles')

    plt.ylabel(" Time to add() complete dataset [s]")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Input Performance")
    plt.legend(loc='best')
    plt.show()
    # plt.savefig("input_performance_5g.png")

def err_bar_dd_eq_sort_merge(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=2"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=2')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=4"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=4')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=8"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=8')

    # eq done
    y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_sort_s_", ["no"])
    yerr_eq = [y_eq_max, y_eq_min]
    plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles (PSort)')

    plt.ylabel(" Time to sort/merge complete dataset [s]")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Sort vs Merge Performance")
    plt.legend(loc='best')
    plt.show()
    # plt.savefig("sort_merge_performance_5g.png")


def err_bar_dd_eq_qv(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_get_qv_ms_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_get_qv_ms_", ["num_threads=2"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=2')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_get_qv_ms_", ["num_threads=4"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=4')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_get_qv_ms_", ["num_threads=8"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=8')

    # eq done
    y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_get_qv_ms_", ["no"])
    yerr_eq = [y_eq_max, y_eq_min]
    plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles (PSort)')

    plt.ylabel(" Time to access complete dataset [ms]")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Quantile values")
    plt.legend(loc='best')
    plt.show()
    # plt.savefig("qv_performance_5g.png")

def err_bar_dd_eq_space(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=2"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=2')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=4"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=4')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=8"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=8')

    # eq done
    y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "space_", ["no"])
    print("eq avg {} \n eq max{}".format(y_eq_avg, y_eq_max))
    yerr_eq = [y_eq_max, y_eq_min]
    plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles (PSort)')

    plt.ylabel(" Space requirements in # of doubles (x8 for Gb)")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Space Requirements")
    plt.legend(loc='best')
    plt.show()
    # plt.savefig("space_performance_5g.png")

def err_bar_acc_dd_eq(dd_squashed, eq_squashed):
    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=2"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=2')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=4"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=4')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=8"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=8')

    # eq done
    y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "space_", ["no"])
    print("eq avg {} \n eq max{}".format(y_eq_avg, y_eq_max))
    yerr_eq = [y_eq_max, y_eq_min]
    plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles (PSort)')

    plt.ylabel(" Space requirements in # of doubles (x8 for Gb)")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Space Requirements")
    plt.legend(loc='best')
    plt.show()
    # plt.savefig("space_performance_5g.png")

def uniform_test():
    qv = [0.5, 0.75, 0.9, 0.95, 0.99, 1]
    range = [-1, 1]
    # ->

# works with bigger values due to being run on server
def serv_err_bar_dd_eq_input(dd_squashed, eq_squashed):
    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=4"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=4')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=32"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=32')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_s_", ["num_threads=96"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=96')

    # eq done
    y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_add_s_", ["no"])
    yerr_eq = [y_eq_max, y_eq_min]
    plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles')

    plt.ylabel(" Time to add() complete dataset [s]")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Input Performance")
    plt.legend(loc='best')
    # plt.show()
    # plt.savefig("input_performance_60g_all.png")


# works with bigger values due to being run on server
def serv_err_bar_dd_eq_sort_merge(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=4"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=4')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=32"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=32')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_s_", ["num_threads=96"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=96')

    # eq done
    # y_eq_min, y_eq_avg, y_eq_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_sort_s_", ["no"])
    # yerr_eq = [y_eq_max, y_eq_min]
    # plt.errorbar(x, y_eq_avg, yerr=yerr_eq, color='darkmagenta', label='Exact Quantiles (PSort)')
    #
    plt.ylabel(" Time to sort/merge complete dataset [s]")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch vs Exact quantiles: Sort vs Merge Performance")
    plt.legend(loc='best')
    # plt.show()
    plt.savefig("sort_merge_performance_60g_dd_only.png")


def serv_err_bar_dd_eq_sort_only(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_ms_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_ms_", ["num_threads=4"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=4')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_ms_", ["num_threads=32"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=32')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_ms_", ["num_threads=96"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=96')

    plt.ylabel(" Time to sort/merge complete dataset [ms]")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch Merge Performance")
    plt.legend(loc='best')
    # plt.show()
    # plt.savefig("merge_performance_60g_dd_only.png")


def serv_err_bar_dd_only_space(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=1"])
    yerr_1 = [y_1_max, y_1_min]
    print(" DDSKETCH DONE")
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch threads=1')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=4"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch threads=4')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=32"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch threads=32')

    y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=96"])
    yerr_8 = [y_8_max, y_8_min]
    plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=96')

    plt.ylabel(" Space requirements in # of doubles (x8 for bytes)")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch: Space Requirements")
    plt.legend(loc='best')
    # plt.show()
    plt.savefig("space_requirements_60g_dd_only.png")


def serv_err_bar_func_comparison(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_add_ms_", ["num_threads=96"])
    yerr_1 = [y_1_max, y_1_min]
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='DDsketch add()')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_merge_ms_", ["num_threads=96"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='DDsketch merge()')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", dd_squashed, "t_get_qv_ms_", ["num_threads=96"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='DDsketch get_qv()')

    # y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=96"])
    # yerr_8 = [y_8_max, y_8_min]
    # plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=96')

    plt.ylabel(" DDSketch function comparison (ms)")
    plt.xlabel("Data set [Gb]")
    plt.title("DDSketch: Execution speed comparison")
    plt.legend(loc='best')
    # plt.show()
    plt.savefig("ddsketch_func_speed_comparison.png")


def serv_err_bar_func_comparison_eq(dd_squashed, eq_squashed):

    fig = plt.figure()
    x = find_series_in_squash(eq_squashed, "size")
    y_1_min, y_1_avg, y_1_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_add_ms_", ["no"])
    yerr_1 = [y_1_max, y_1_min]
    plt.errorbar(x, y_1_avg, yerr=yerr_1, color='orange', label='Exact Quantiles add()')
    y_2_min, y_2_avg, y_2_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_sort_ms_", ["no"])
    yerr_2 = [y_2_max, y_2_min]
    plt.errorbar(x, y_2_avg, yerr=yerr_2, color='deepskyblue', label='Exact Quantiles sort()')


    y_4_min, y_4_avg, y_4_max = filter_yseries_in_squash(x, "size", eq_squashed, "t_get_qv_ms_", ["no"])
    yerr_4 = [y_4_max, y_4_min]
    plt.errorbar(x, y_4_avg, yerr=yerr_4, color='dodgerblue', label='Exact Quantiles get_qv()')

    # y_8_min, y_8_avg, y_8_max = filter_yseries_in_squash(x, "size", dd_squashed, "space_", ["num_threads=96"])
    # yerr_8 = [y_8_max, y_8_min]
    # plt.errorbar(x, y_8_avg, yerr=yerr_8, color='darkgreen', label='DDsketch threads=96')

    plt.ylabel(" DDSketch function comparison (ms)")
    plt.xlabel("Data set [Gb]")
    plt.title("Exact Quantiles: Execution speed comparison")
    plt.legend(loc='best')
    # plt.show()
    plt.savefig("eq_func_speed_comparison.png")



def main():
    # benchmark log path: first CLI argument, with a sensible default
    import sys
    infile = sys.argv[1] if len(sys.argv) > 1 else "benchmark_log.txt"
    unsorted = readfile(infile)
    dd_runs = filter_list(unsorted, "alg=dd")
    dd_combined = combine_similar(dd_runs, ["size=", "num_threads="])  # each listitem in dd_combined is a list of runs with same size and num_threads
    dd_squashed = squash_nine(dd_combined, ["alg", "run", "size", "num_threads", "qv1"])  # 2nd param is what shouldnt be min/max/averaged
    # at 2ds-sizes, 4 values of threads and 9 runs each, we should go from 72 to 2 * 4 = 8
    print("dd-crunching done")
    # the C++ harness tags exact-quantile runs as "alg=sbeq" (not "alg=eq")
    eq_runs = filter_list(unsorted, "alg=sbeq")
    eq_combined = combine_similar(eq_runs, ["size="])  # without threads, we only have 2ds-sizes atm
    eq_squashed = squash_nine(eq_combined, ["alg", "run", "size", "qv1"]) # 2nd param is what shouldnt be min/max/averaged

    """
    err_bar_dd_eq_input(dd_squashed, eq_squashed)
    err_bar_dd_eq_sort_merge(dd_squashed, eq_squashed)
    err_bar_dd_eq_qv(dd_squashed, eq_squashed)
    err_bar_dd_eq_space(dd_squashed, eq_squashed)
    err_bar_acc_dd_eq(dd_squashed, eq_squashed)
    """
    # serv_err_bar_dd_eq_input(dd_squashed, eq_squashed)
    # serv_err_bar_dd_eq_sort_merge(dd_squashed, eq_squashed)
    # serv_err_bar_dd_eq_sort_only(dd_squashed, eq_squashed)
    # serv_err_bar_dd_only_space(dd_squashed, eq_squashed)
    # serv_err_bar_func_comparison(dd_squashed, eq_squashed)
    serv_err_bar_func_comparison_eq(dd_squashed, eq_squashed)

if __name__ == '__main__':
    print_hi('PyCharm')
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
