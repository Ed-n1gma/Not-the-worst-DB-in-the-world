
select
        l.l_orderkey
from
        lineitem as l
where
        (l.l_shipinstruct = "TAKE BACK RETURN") and
        (l.l_extendedprice / l.l_quantity > 1759.6) and
        (l.l_extendedprice / l.l_quantity < 1759.8);
