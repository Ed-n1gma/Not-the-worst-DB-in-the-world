
select
        ps.ps_partkey,
        avg (ps.ps_supplycost)
from
        part as p,
        partsupp as ps,
        supplier as s,
        nation as n,
        region as r
where
        (p.p_partkey = ps.ps_partkey)
        and (s.s_suppkey = ps.ps_suppkey)
        and (s.s_nationkey = n.n_nationkey)
        and (n.n_regionkey = r.r_regionkey)
        and (r.r_name = "AMERICA")
group by
        ps.ps_partkey;
