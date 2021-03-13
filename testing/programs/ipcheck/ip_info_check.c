/* ip_address tests, for libreswan
 *
 * Copyright (C) 2000  Henry Spencer.
 * Copyright (C) 2012 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2018 Andrew Cagney
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/lgpl-2.1.txt>.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 */

#include <stdio.h>
#include <string.h>

#include "lswcdefs.h"		/* for elemsof() */
#include "constants.h"		/* for streq() */
#include "ip_address.h"
#include "ip_selector.h"
#include "ip_range.h"
#include "jambuf.h"

#include "ipcheck.h"

#define CHECK_OP(L,OP,R)						\
	for (size_t tl = 0; tl < elemsof(L##_tests); tl++) {		\
		/*hack*/const typeof(L##_tests[0]) *t = &L##_tests[tl];	\
		/*hack*/size_t ti = tl;					\
		const ip_##L *l = L##_tests[tl].L;			\
		if (l == NULL) continue;				\
		for (size_t tr = 0; tr < elemsof(R##_tests); tr++) {	\
			const ip_##R *r = R##_tests[tr].R;		\
			if (r == NULL) continue;			\
			int expected = 0;				\
			for (size_t to = 0; to < elemsof(L##_op_##R); to++) { \
				const typeof(L##_op_##R[0]) *op = &L##_op_##R[to]; \
				if (l == op->l && r == op->r) {		\
					expected = op->OP;		\
					break;				\
				}					\
			}						\
			int b = L##_##OP##_##R(*l, *r);			\
			/* work with int *cmp() */			\
			if ((b > 0 && expected <= 0) ||			\
			    (b == 0 && expected != 0) ||		\
			    (b < 0 && expected >= 0)) {			\
				L##_buf lb;				\
				R##_buf rb;				\
				FAIL(#L "_" #OP "_" #R "(%s,%s) returned %d, expected %d", \
				     str_##L(l, &lb),			\
				     str_##R(r, &rb),			\
				     b, expected);			\
			}						\
		}							\
	}

/*
 * Check equality.  First form assumes NULL allowed, second does
 * not.
 */

#define CHECK_EQ(T)							\
	for (size_t ti = 0; ti < elemsof(T##_tests); ti++) {		\
		const struct T##_test *t = &T##_tests[ti];		\
		const ip_##T *ai = t->T;				\
		for (size_t tj = 0; tj < elemsof(T##_tests); tj++) {	\
			const ip_##T *aj = T##_tests[tj].T;		\
			T##_buf bi, bj;					\
			bool expected = eq[ti][tj];			\
			PRINT("[%zu][%zu] "#T"_eq(%s,%s) == %s",	\
			      ti, tj,					\
			      str_##T(ai, &bi),				\
			      str_##T(aj, &bj),				\
			      bool_str(expected));			\
			bool actual = T##_eq(ai, aj);			\
			if (expected != actual) {			\
				FAIL("[%zu][%zu] "#T"_eq(%s,%s) returned %s, expecting %s", \
				     ti, tj,				\
				     str_##T(ai, &bi),			\
				     str_##T(aj, &bj),			\
				     bool_str(actual),			\
				     bool_str(expected));		\
			}						\
		}							\
	}

#define CHECK_EQ2(T)							\
	for (size_t ti = 0; ti < elemsof(T##_tests); ti++) {		\
		const struct T##_test *t = &T##_tests[ti];		\
		const ip_##T *ai = (T##_tests[ti].T == NULL		\
				    ? &unset_##T			\
				    : T##_tests[ti].T);			\
		for (size_t tj = 0; tj < elemsof(T##_tests); tj++) {	\
			const ip_##T *aj = (T##_tests[tj].T == NULL	\
					    ? &unset_##T		\
					    : T##_tests[tj].T);		\
			T##_buf bi, bj;					\
			bool expected = eq[ti][tj];			\
			PRINT("[%zu][%zu] "#T"_eq(%s,%s) == %s",	\
			      ti, tj,					\
			      str_##T(ai, &bi),				\
			      str_##T(aj, &bj),				\
			      bool_str(expected));			\
			bool actual = T##_eq_##T(*ai, *aj);		\
			if (expected != actual) {			\
				FAIL("[%zu][%zu] "#T"_eq(%s,%s) returned %s, expecting %s", \
				     ti, tj,				\
				     str_##T(ai, &bi),			\
				     str_##T(aj, &bj),			\
				     bool_str(actual),			\
				     bool_str(expected));		\
			}						\
		}							\
	}

static const struct address_test {
	int line;
	int family;
	const ip_address *address;
	const char *str;
	bool is_unset;
	bool is_any;
	bool is_specified;
	bool is_loopback;
} address_tests[] = {
	{ LN, 0, NULL,                        "<unset-address>", .is_unset = true, },
	{ LN, 0, &unset_address,              "<unset-address>", .is_unset = true, },
	{ LN, 4, &ipv4_info.address.any,      "0.0.0.0",         .is_any = true },
	{ LN, 6, &ipv6_info.address.any,      "::",              .is_any = true },
	{ LN, 4, &ipv4_info.address.loopback, "127.0.0.1",       .is_specified = true, .is_loopback = true, },
	{ LN, 6, &ipv6_info.address.loopback, "::1",             .is_specified = true, .is_loopback = true, },
};

static const struct subnet_test {
	int line;
	int family;
	const ip_subnet *subnet;
	const char *str;
	bool is_unset;
	bool is_specified;
	bool contains_no_addresses;
	bool contains_one_address;
	bool contains_all_addresses;
} subnet_tests[] = {
	{ LN, 0, NULL,                    "<unset-subnet>", .is_unset = true, },
	{ LN, 0, &unset_subnet,           "<unset-subnet>", .is_unset = true, },
	{ LN, 4, &ipv4_info.subnet.none,  "0.0.0.0/32",     .contains_no_addresses = true, },
	{ LN, 6, &ipv6_info.subnet.none,  "::/128",         .contains_no_addresses = true, },
	{ LN, 4, &ipv4_info.subnet.all,   "0.0.0.0/0",      .contains_all_addresses = true, },
	{ LN, 6, &ipv6_info.subnet.all,   "::/0",           .contains_all_addresses = true, },
};

static const struct range_test {
	int line;
	int family;
	const ip_range *range;
	const char *str;
	bool is_unset;
	bool is_specified;
	bool contains_no_addresses;
	bool contains_one_address;
	bool contains_all_addresses;
} range_tests[] = {
	{ LN, 0, NULL,                  "<unset-range>", .is_unset = true, },
	{ LN, 0, &unset_range,          "<unset-range>", .is_unset = true, },
	{ LN, 4, &ipv4_info.range.none, "0.0.0.0-0.0.0.0",  .contains_no_addresses = true, },
	{ LN, 6, &ipv6_info.range.none, "::-::",            .contains_no_addresses = true, },
	{ LN, 4, &ipv4_info.range.all,  "0.0.0.0-255.255.255.255",    .contains_all_addresses = true, },
	{ LN, 6, &ipv6_info.range.all,  "::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", .contains_all_addresses = true, },
};

static const struct endpoint_test {
	int line;
	int family;
	const ip_endpoint *endpoint;
	const char *str;
	bool is_unset;
	bool is_specified;
	bool is_any;
	int hport;
} endpoint_tests[] = {
	{ LN, 0, NULL,                     "<unset-endpoint>",   .is_unset = true, .hport = -1, },
	{ LN, 0, &unset_endpoint,          "<unset-endpoint>",   .is_unset = true, .hport = -1, },
};

static const struct selector_test {
	int line;
	int family;
	const ip_selector *selector;
	const char *str;
	bool is_unset;
	bool is_specified;
	bool contains_all_addresses;
	bool is_one_address;
	bool contains_no_addresses;
} selector_tests[] = {
	{ LN, 0, NULL,                     "<unset-selector>", .is_unset = true, },
	{ LN, 0, &unset_selector,          "<unset-selector>", .is_unset = true, },
	{ LN, 4, &ipv4_info.selector.none, "0.0.0.0/32",       .contains_no_addresses = true, },
	{ LN, 6, &ipv6_info.selector.none, "::/128",           .contains_no_addresses = true, },
	{ LN, 4, &ipv4_info.selector.all,  "0.0.0.0/0",        .contains_all_addresses = true, },
	{ LN, 6, &ipv6_info.selector.all,  "::/0",             .contains_all_addresses = true, },
};

static void check_ip_info_address(void)
{
	for (size_t ti = 0; ti < elemsof(address_tests); ti++) {
		const struct address_test *t = &address_tests[ti];
		PRINT("%s", pri_family(t->family));

		const ip_address *address = t->address;
		CHECK_TYPE(address_type(address));

		CHECK_STR2(address);

		CHECK_COND(address, is_unset);
		CHECK_COND2(address, is_any);
		CHECK_COND2(address, is_specified);
		CHECK_COND2(address, is_loopback);
	}

	static const struct {
		const ip_address *l;
		const ip_address *r;
		int eq;
	} address_op_address[] = {
		/* any */
		{ &unset_address,              &unset_address,        .eq = true, },
		{ &ipv4_info.address.any,      &ipv4_info.address.any, .eq = true, },
		{ &ipv6_info.address.any,      &ipv6_info.address.any, .eq = true, },
		{ &ipv4_info.address.loopback, &ipv4_info.address.loopback, .eq = true, },
		{ &ipv6_info.address.loopback, &ipv6_info.address.loopback, .eq = true, },
	};

	CHECK_OP(address, eq, address);
}

static void check_ip_info_endpoint(void)
{
	for (size_t ti = 0; ti < elemsof(endpoint_tests); ti++) {
		const struct endpoint_test *t = &endpoint_tests[ti];
		PRINT("%s", pri_family(t->family));

		const ip_endpoint *endpoint = t->endpoint;
		CHECK_TYPE(endpoint_type(endpoint));

		CHECK_STR2(endpoint);

		CHECK_COND(endpoint, is_unset);
		CHECK_COND(endpoint, is_specified);

		if (!t->is_unset) {
			int hport = endpoint_hport(endpoint);
			if (hport != t->hport) {
				FAIL(" endpoint_port() returned %d, expecting %d",
				     hport, t->hport);
			}
		}
	}

	/* must match table above */
	bool eq[elemsof(endpoint_tests)][elemsof(endpoint_tests)] = {
		/* unset/NULL */
		[0][0] = true,
		[0][1] = true,
		[1][1] = true,
		[1][0] = true,
	};
	CHECK_EQ(endpoint);
}

static void check_ip_info_subnet(void)
{
	for (size_t ti = 0; ti < elemsof(subnet_tests); ti++) {
		const struct subnet_test *t = &subnet_tests[ti];
		PRINT("%s unset=%s all=%s some=%s none=%s",
		      pri_family(t->family),
		      bool_str(t->is_unset),
		      bool_str(t->contains_all_addresses),
		      bool_str(t->is_specified),
		      bool_str(t->contains_no_addresses));

		const ip_subnet *subnet = t->subnet;
		if (t->family != 0) {
			CHECK_TYPE(subnet_type(subnet));
		}

		CHECK_STR2(subnet);

		CHECK_COND(subnet, is_unset);

		if (subnet == NULL) continue;

		CHECK_COND2(subnet, is_specified);
		CHECK_COND2(subnet, contains_no_addresses);
		CHECK_COND2(subnet, contains_one_address);
		CHECK_COND2(subnet, contains_all_addresses);

	}

	static const struct {
		const ip_subnet *l;
		const ip_subnet *r;
		int eq;
		int in;
	} subnet_op_subnet[] = {
		/* any */
		{ &unset_subnet, &unset_subnet, .eq = true, },
		/* none in none */
		{ &ipv4_info.subnet.none, &ipv4_info.subnet.none, .eq = true, .in = true, },
		{ &ipv6_info.subnet.none, &ipv6_info.subnet.none, .eq = true, .in = true, },
		/* all in all */
		{ &ipv4_info.subnet.all,  &ipv4_info.subnet.all,  .eq = true, .in = true, },
		{ &ipv6_info.subnet.all,  &ipv6_info.subnet.all,  .eq = true, .in = true, },
		/* none in all */
		{ &ipv4_info.subnet.none,  &ipv4_info.subnet.all,             .in = true, },
		{ &ipv6_info.subnet.none,  &ipv6_info.subnet.all,             .in = true, },
	};

	CHECK_OP(subnet, eq, subnet);
	CHECK_OP(subnet, in, subnet);

	static const struct {
		const ip_subnet *l;
		const ip_address *r;
		int eq;
	} subnet_op_address[] = {
		{ &ipv4_info.subnet.none, &ipv4_info.address.any, .eq = true, },
		{ &ipv6_info.subnet.none, &ipv6_info.address.any, .eq = true, },
	};

	CHECK_OP(subnet, eq, address);

	static const struct {
		const ip_address *l;
		const ip_subnet *r;
		int in;
	} address_op_subnet[] = {
		{ &ipv4_info.address.any,      &ipv4_info.subnet.all, .in = true, },
		{ &ipv6_info.address.any,      &ipv6_info.subnet.all, .in = true, },
		{ &ipv4_info.address.loopback, &ipv4_info.subnet.all, .in = true, },
		{ &ipv6_info.address.loopback, &ipv6_info.subnet.all, .in = true, },
	};
	CHECK_OP(address, in, subnet);

}

static void check_ip_info_range(void)
{
	for (size_t ti = 0; ti < elemsof(range_tests); ti++) {
		const struct range_test *t = &range_tests[ti];
		PRINT("%s", pri_family(t->family));

		const ip_range *range = t->range;
		CHECK_TYPE(range_type(range));

		CHECK_STR2(range);

		CHECK_COND(range, is_unset);
		CHECK_COND2(range, is_specified);
	}

	/* must match table above */
	bool eq[elemsof(range_tests)][elemsof(range_tests)] = {
		/* unset/NULL */
		[0][0] = true,
		[0][1] = true,
		[1][1] = true,
		[1][0] = true,
		/* other */
		[2][2] = true,
		[3][3] = true,
		[4][4] = true,
		[5][5] = true,
	};
	CHECK_EQ2(range);
}

static void check_ip_info_selector(void)
{
	for (size_t ti = 0; ti < elemsof(selector_tests); ti++) {
		const struct selector_test *t = &selector_tests[ti];
		PRINT("%s", pri_family(t->family));

		const ip_selector *selector = t->selector;
		CHECK_TYPE(selector_type(selector));

		CHECK_STR2(selector);

		CHECK_COND(selector, is_unset);
		CHECK_COND(selector, contains_no_addresses);
		CHECK_COND(selector, contains_all_addresses);
	}

	/* must match table above */
	bool eq[elemsof(selector_tests)][elemsof(selector_tests)] = {
		/* unset/NULL */
		[0][0] = true,
		[0][1] = true,
		[1][1] = true,
		[1][0] = true,
		/* other */
		[2][2] = true,
		[3][3] = true,
		[4][4] = true,
		[5][5] = true,
	};
	CHECK_EQ(selector);
}

void ip_info_check(void)
{
	check_ip_info_address();
	check_ip_info_endpoint();
	check_ip_info_subnet();
	check_ip_info_range();
	check_ip_info_selector();
}
