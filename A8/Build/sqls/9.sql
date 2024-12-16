
select
        "name: " + s.s_name,
        sum (1)
from
        supplier as s,
        lineitem as l1,
        lineitem as l2,
        orders as o,
        nation as n
where
        (s.s_suppkey = l1.l_suppkey)
        and (o.o_orderkey = l1.l_orderkey)
	and (l2.l_shipinstruct = "TAKE BACK RETURN") 
        and (o.o_orderstatus = "F")
        and (l1.l_receiptdate > l1.l_commitdate)
	and (o.o_orderpriority < "2-HIGH" or o.o_orderpriority = "2-HIGH")
        and (l2.l_orderkey = l1.l_orderkey)
        and (not l2.l_suppkey = l1.l_suppkey)
group by
        s.s_name;
