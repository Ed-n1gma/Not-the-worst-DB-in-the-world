
select
        l.l_comment
from
        lineitem as l
where
        (l.l_shipdate = "1994-05-12") and
        (l.l_commitdate = "1994-05-22") and
        (l.l_receiptdate = "1994-06-10");
