# COP 6726 - Project 4(Part II)
Query Compilation and Optimization

## Group Info
  - Anand Chinnappan Mani,  UFID: 7399-9125
  - Utkarsh Roy,            UFID: 9109-6657

## Instructions
```
make clean 
make main
./main
```
## Test cases
Enter CNF and press ctrl+D

CNF -	SELECT SUM (ps.ps_supplycost)
		FROM part AS p, supplier AS s, partsupp AS ps
		WHERE (p.p_partkey = ps.ps_partkey) AND
		(s.s_acctbal > 2500.0)

## Instructions for GTest

```
make clean 
make gtest.out
./gtest.out
```

## GTest

Run unit test for separate modules of the Query Plan


