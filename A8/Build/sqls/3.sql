
select
        sum (1),
        avg ((o.o_totalprice - 32592.14) / 32592.14)
from
        orders as o
where
        (o.o_orderstatus = "F") and
        (o.o_orderpriority < "2-HIGH" or o.o_orderpriority = "2-HIGH");
