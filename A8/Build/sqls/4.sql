
select
        avg (o.o_totalprice),
        "priority: " + o.o_orderpriority
from
        orders as o
where
        (o.o_orderstatus = "F")
group by
        o.o_orderpriority;
