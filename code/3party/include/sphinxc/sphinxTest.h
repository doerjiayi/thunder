//
// $Id$
//

//
// Copyright (c) 2001-2011, Andrew Aksyonoff
// Copyright (c) 2008-2011, Sphinx Technologies Inc
// All rights reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License. You should
// have received a copy of the LGPL license along with this program; if you
// did not, you can find it at http://www.gnu.org/
//
#ifndef _SPHINX_TEST_
#define _SPHINX_TEST_
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#include <winsock2.h>
#endif

#include "../sphinxc/sphinxclient.h"

#ifndef STRING_DEFINE
#define STRING_DEFINE(x) #x
#endif
#ifndef TOSTRING
#define TOSTRING(x) STRING_DEFINE(x)
#endif
#ifndef AT_FILE
#define AT_FILE __FILE__"(" TOSTRING(__LINE__) "):"
#endif

static int g_failed = 0;

#ifdef  __cplusplus
extern "C" {
#endif

#define debug_log(fmt, ...) __log_debug(AT_FILE fmt , ##__VA_ARGS__)

#define fail_debug_log(fmt, ...) do{__log_debug(AT_FILE fmt , ##__VA_ARGS__);return;}while(0)

void __log_debug ( const char * temp, ... )
{
    va_list ap;
    va_start ( ap, temp );
    vprintf ( temp, ap );
    printf ( "\n" );
    va_end ( ap );
}

void net_init ()
{
#if _WIN32
	// init WSA on Windows
	WSADATA wsa_data;
	int wsa_startup_err;

	wsa_startup_err = WSAStartup ( WINSOCK_VERSION, &wsa_data );
	if ( wsa_startup_err )
	    debug_log ( "failed to initialize WinSock2: error %d", wsa_startup_err );
#endif
}


void test_query ( sphinx_client * client, const char * query, const char *index)
{
	sphinx_result * res;
	int i, j, k, mva_len;
	unsigned int * mva;
	const char * field_names[2];
	int field_weights[2];
	field_names[0] = "req_type";
	field_names[1] = "create_date";
	field_weights[0] = 100;
	field_weights[1] = 1;
	sphinx_set_field_weights ( client, 2, field_names, field_weights );
	field_weights[0] = 1;
	field_weights[1] = 1;
	res = sphinx_query ( client, query, index, NULL );
	if ( !res )
	{
		g_failed += ( res==NULL );
		fail_debug_log ( "query failed: %s", sphinx_error(client) );
	}

	debug_log ( "Query '%s' retrieved %d of %d matches in %d.%03d sec.\n",
			query, res->total, res->total_found, res->time_msec/1000, res->time_msec%1000 );
    //	../../src/sphinxTest.h(327):------------ 测试查询单词:问题1------------
    //	../../src/sphinxTest.h(92):Query '问题1' retrieved 1 of 1 matches in 0.003 sec.
    //	../../src/sphinxTest.h(94):Query stats:
    //	../../src/sphinxTest.h(97):     '问题' found 3 times in 3 documents
    //	../../src/sphinxTest.h(97):     '1' found 1 times in 1 documents
	debug_log ( "Query stats:\n" );
	for ( i=0; i<res->num_words; i++ )
	    debug_log ( "\t'%s' found %d times in %d documents\n",
		res->words[i].word, res->words[i].hits, res->words[i].docs );

	debug_log ( "\nMatches:\n" );
    //	Matches:
    //	../../src/sphinxTest.h(103):1. doc_id=3, weight=2, res=答案1, req_type=123, create_date=1479780177, update_date=1479780177
	for ( i=0; i<res->num_matches; i++ )
	{
	    debug_log ( "%d. doc_id=%d, weight=%d", 1+i,
			(int)sphinx_get_id ( res, i ), sphinx_get_weight ( res, i ) );

		for ( j=0; j<res->num_attrs; j++ )
		{
			printf ( ", %s=", res->attr_names[j] );
			switch ( res->attr_types[j] )
			{
			case SPH_ATTR_MULTI64:
			case SPH_ATTR_MULTI:
				mva = sphinx_get_mva ( res, i, j );
				mva_len = *mva++;
				printf ( "(" );
				for ( k=0; k<mva_len; k++ )
					printf ( k ? ",%u" : "%u", ( res->attr_types[j]==SPH_ATTR_MULTI ? mva[k] : (unsigned int)sphinx_get_mva64_value ( mva, k ) ) );
				printf ( ")" );
				break;

			case SPH_ATTR_FLOAT:	printf ( "%f", sphinx_get_float ( res, i, j ) ); break;
			case SPH_ATTR_STRING:	printf ( "%s", sphinx_get_string ( res, i, j ) ); break;
			default:				printf ( "%u", (unsigned int)sphinx_get_int ( res, i, j ) ); break;
			}
		}

		printf ( "\n" );
	}
	printf ( "\n" );
}


void test_excerpt ( sphinx_client * client ,const char * words, const char * index)
{
	const char * docs[] =
	{
		"this is my test text to be highlighted, and for the sake of the testing we need to pump its length somewhat",
		"another test text to be highlighted, below limit",
		"test number three, without phrase match",
		"final test, not only without phrase match, but also above limit and with swapped phrase text test as well"
	};
	const int ndocs = sizeof(docs)/sizeof(docs[0]);
	sphinx_excerpt_options opts;
	char ** res;
	int i, j;

	sphinx_init_excerpt_options ( &opts );
	opts.limit = 60;
	opts.around = 3;
	opts.allow_empty = SPH_TRUE;

	for ( j=0; j<2; j++ )
	{
		opts.exact_phrase = j;
		debug_log ( "exact_phrase=%d\n", j );
		res = sphinx_build_excerpts ( client, ndocs, docs, index, words, &opts );
		if ( !res )
		{
			g_failed += ( res==NULL );
		    fail_debug_log ( "query failed: %s", sphinx_error(client) );
		}
		for ( i=0; i<ndocs; i++ )
		    debug_log ( "n=%d, res=%s\n", 1+i, res[i] );
		printf ( "\n" );
	}
}


void test_excerpt_spz ( sphinx_client * client ,const char * words,const char * index)
{
	const char * docs[] =
	{
		"<efx_unidentified_table>"
		"The institutional investment manager it. Is Filing this report and."
		"<efx_test>"
		"It is signed hereby represent. That it is all information."
		"are It or is"
		"</efx_test>"
		"<efx_2>"
		"cool It is cooler"
		"</efx_2>"
		"It is another place!"
		"</efx_unidentified_table>"
	};
	const int ndocs = sizeof(docs)/sizeof(docs[0]);
	sphinx_excerpt_options opts;
	char ** res;
	int i, j;

	sphinx_init_excerpt_options ( &opts );
	opts.limit = 150;
	opts.limit_passages = 8;
	opts.around = 8;
	opts.html_strip_mode = "strip";
	opts.passage_boundary = "zone";
	opts.emit_zones = SPH_TRUE;

	for ( j=0; j<2; j++ )
	{
		if ( j==1 )
		{
			opts.passage_boundary = "sentence";
			opts.emit_zones = SPH_FALSE;
		}
		debug_log ( "passage_boundary=%s\n", opts.passage_boundary );
		res = sphinx_build_excerpts ( client, ndocs, docs, index, words, &opts );
		if ( !res )
		    fail_debug_log ( "query failed: %s", sphinx_error(client) );

		for ( i=0; i<ndocs; i++ )
		    debug_log ( "n=%d, res=%s\n", 1+i, res[i] );
		printf ( "\n" );
	}
}


void test_update ( sphinx_client * client, sphinx_uint64_t id ,const char * attr,const sphinx_int64_t val,const char* index)
{
	int res = sphinx_update_attributes ( client, index, 1, &attr, 1, &id, &val );
	if ( res<0 )
		g_failed++;
	if ( res<0 )
	    fail_debug_log ( "update failed: %s\n\n", sphinx_error(client));
	else
	    debug_log ( "update success, %d rows updated\n\n", res );
}

void test_update_mva ( sphinx_client * client ,const sphinx_uint64_t id,const char * attr,const char* index)
{
	const unsigned int vals[] = { 7, 77, 177 };
	int res = sphinx_update_attributes_mva ( client, index, attr, id, sizeof(vals)/sizeof(vals[0]), vals );
	if ( res<0 )
		g_failed++;
	if ( res<0 )
	    fail_debug_log("update mva failed: %s\n\n", sphinx_error(client));
	else
	    debug_log( "update mva success, %d rows updated\n\n", res );
}


void test_keywords ( sphinx_client * client ,const char* query,const char* index)
{
	int i, nwords;
	sphinx_keyword_info * words;
	words = sphinx_build_keywords ( client, query, index, SPH_TRUE, &nwords );
	g_failed += ( words==NULL );
	if ( !words )
	{
	    fail_debug_log ( "build_keywords failed: %s\n\n", sphinx_error(client) );
	}
	else
	{
	    debug_log ( "build_keywords result:\n" );
		for ( i=0; i<nwords; i++ )
		    debug_log ( "%d. tokenized=%s, normalized=%s, docs=%d, hits=%d\n", 1+i,
				words[i].tokenized, words[i].normalized,
				words[i].num_docs, words[i].num_hits );
	}
}


void test_status ( sphinx_client * client )
{
	int num_rows, num_cols, i, j, k;
	char ** status;

	status = sphinx_status ( client, &num_rows, &num_cols );
	if ( !status )
	{
		g_failed++;
		fail_debug_log( "status failed: %s\n\n", sphinx_error(client) );
		return;
	}

	k = 0;
	for ( i=0; i<num_rows; i++ )
	{
		if ( ( strstr ( status[k], "time" )==NULL && strstr ( status[k], "wall" )==NULL ) )
		{
			for ( j=0; j<num_cols; j++, k++ )
			    printf ( ( j==0 ) ? "%s:" : " %s", status[k] );
		} else
			k += num_cols;
	}
	sphinx_status_destroy ( status, num_rows, num_cols );
}

void test_group_by ( sphinx_client * client, const char * attr ,const char* index)
{
	sphinx_set_groupby ( client, attr, SPH_GROUPBY_ATTR, "@group asc" );
	test_query ( client, "is" ,index);
	sphinx_reset_groupby ( client );
}

void test_filter ( sphinx_client * client ,const char * attr,const char* query,const char* index)
{
    sphinx_int64_t filter_group = { 1 };
    sphinx_add_filter ( client, attr, 1, &filter_group, SPH_FALSE );
    test_query ( client, query ,index);
    sphinx_reset_filters ( client );
}

void title ( const char * name )
{
	debug_log ( "-> % s <-\n\n", name );
}

int testSphinx ( const char* host,int port ,const char* index = "main")
{
	sphinx_client * client;
	net_init ();
	client = sphinx_create ( SPH_TRUE );
	if ( !client )
	    debug_log ( "failed to create client" );
	if ( port )
		sphinx_set_server ( client, host, port );

	sphinx_set_match_mode ( client, SPH_MATCH_PHRASE );
	sphinx_set_sort_mode ( client, SPH_SORT_RELEVANCE, NULL );

	debug_log("------------ 测试查询片段 :%s------------","test text");
	test_excerpt ( client ,"test text" , index);

	debug_log("------------ 测试查询片段 2:%s------------","test text");
	test_excerpt_spz ( client ,"test text" , index);

	debug_log("------------ 测试统计分词信息:%s------------","hello test one");
	test_keywords ( client ,"hello test one",index);

	debug_log("------------ 测试查询单词:%s------------","问题1");
	test_query ( client, "问题1" ,index);

	debug_log("------------ 测试查询单词(过滤条件:%s):%s------------","req_type",index);
    test_filter ( client ,"req_type" ,"问题1",index);

	debug_log("------------ 测试查询多个单词:%s------------","this is my test document % number  one");
	test_query ( client, "this is my test document % number  one",index);

	// persistence connection
	sphinx_open ( client );
	debug_log("------------ 测试更新------------");
	// update (attr) + sort (default)
	test_update ( client, 4 ,"req_type" ,1234,index);
	test_update ( client, 3 ,"req_type" ,123,index);
	sphinx_set_sort_mode ( client, SPH_SORT_RELEVANCE, NULL );
	sphinx_cleanup ( client );

	// select
	debug_log ( "--列出具体需要获取的属性，和需要计算并获取的表达式 *, req_type*1000+@id*10 AS q" );
	sphinx_set_select ( client, "*, req_type*1000+@id*10 AS q" );

	debug_log("------------ 测试查询单词:%s------------","问题1");
	test_query ( client, "问题1" ,index);

	debug_log("------------ 关闭连接 ------------");
	sphinx_close ( client );
	debug_log("------------ test_status ( client )------------");
	test_status ( client );
	sphinx_destroy ( client );
	if ( g_failed )
	{
	    debug_log("failed time:%d", g_failed );
	}
	return 0;
}

#ifdef  __cplusplus
}
#endif

//
// $Id$
//

#endif
