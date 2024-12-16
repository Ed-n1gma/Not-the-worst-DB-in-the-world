
select
        "date was " + o.o_orderdate
from
        supplier as s,
        orders as o,
        customer as c,
        nation as n1,
        nation as n2,
        region as r1,
        region as r2
where
        (o.o_custkey = c.c_custkey)
        and (c.c_nationkey = n1.n_nationkey or c.c_nationkey = n2.n_nationkey)
        and (n1.n_regionkey = r1.r_regionkey)
        and (r1.r_name = "AMERICA")
        and (n2.n_regionkey = r2.r_regionkey)
        and (r2.r_name = "MIDDLE EAST")
        and (s.s_nationkey = n2.n_nationkey)
	and (o.o_orderstatus = "F")
        and (o.o_orderpriority < "2-HIGH" or o.o_orderpriority = "2-HIGH")
        and (o.o_orderdate > "1995-01-01" or o.o_orderdate = "1995-01-01")
        and (o.o_orderdate < "1996-12-31" or o.o_orderdate = "1996-12-31")
group by
        o.o_orderdate;

