
select
        n.n_name,
        o.o_orderdate,
        l.l_extendedprice * (1 - l.l_discount) - ps.ps_supplycost * l.l_quantity
from
        part as p,
        supplier as s,
        lineitem as l,
        partsupp as ps,
        orders as o,
        nation as n
where
        (s.s_suppkey = l.l_suppkey)
        and (ps.ps_suppkey = l.l_suppkey)
        and (ps.ps_partkey = l.l_partkey)
        and (p.p_partkey = l.l_partkey)
        and (o.o_orderkey = l.l_orderkey)
	and (l.l_shipmode = "TRUCK" or l.l_shipmode = "RAIL")
	and (l.l_shipdate < "1998-12-01") 
        and (l.l_shipdate > "1998-06-01")
        and (s.s_nationkey = n.n_nationkey)
        and (p.p_type = "STANDARD POLISHED TIN" or
             p.p_type = "ECONOMY BRUSHED TIN" or
             p.p_type = "ECONOMY POLISHED NICKEL" or
             p.p_type = "SMALL ANODIZED COPPER");
