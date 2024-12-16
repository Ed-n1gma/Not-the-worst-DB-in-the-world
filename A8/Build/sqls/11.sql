
select
        c.c_name,
        n.n_name,
        sum (1)
from
        orders as o,
        lineitem as l1,
        lineitem as l2,
        customer as c,
        nation as n
where
        (o.o_orderkey = l1.l_orderkey)
        and (o.o_orderkey = l2.l_orderkey)
        and (o.o_orderstatus = "F")
        and (l1.l_shipmode = "TRUCK")
        and (c.c_custkey = o.o_custkey)
        and (l2.l_shipmode = "RAIL")
        and (n.n_nationkey = c.c_nationkey)
group by
        n.n_name, c.c_name;
