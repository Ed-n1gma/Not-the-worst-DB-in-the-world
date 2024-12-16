
select
        c2.c_name,
        p.p_name,
        sum (1)
from
        part as p,
        orders as o1,
        orders as o2,
        lineitem as l1,
        lineitem as l2,
        customer as c1,
        customer as c2
where
        (l1.l_partkey = l2.l_partkey)
        and (o1.o_orderkey = l1.l_orderkey)
        and (o2.o_orderkey = l2.l_orderkey)
        and (o1.o_custkey = c1.c_custkey)
        and (o2.o_custkey = c2.c_custkey)
        and (c1.c_name = "Customer#000129200")
        and (p.p_partkey = l2.l_partkey)
group by
        c2.c_name,
        p.p_name;
