var frag_base = 'objs/';
var obj_base = 'tree/';

var frag_t = {};
var frag_l = {};
var frag_s = {};
var xr_obj = {};

var expands = {};

var tags;

var xr_cw;
var xr_ch;
var lno_w;
var lno_pad;

var top_expansion_target;

var init_line;

var search_open;

function url()
{
	var hash = window.location.hash;
	var colon = hash.indexOf(':');
	if (colon != -1)
		return window.location.hash.substring(1, colon);
	else
		return window.location.hash.substring(1);
}

function url_line() {
	var hash = window.location.hash;
	var colon = hash.indexOf(':');
	if (colon != -1)
		return parseInt(window.location.hash.substring(colon+1));
	else
		return undefined;
}

function xr_frag_list(name) {
   var frag_list;
   switch (name.charAt(0)) {
      case 't':
         frag_list = frag_t;
         break;
      case 'l':
         frag_list = frag_l;
         break;
      case 's':
         frag_list = frag_s;
         break;
   }
   return frag_list;
}

function xr_list_insert(list, name, hi, obj)
{
   if (name in list && list[name].ready) {
		list[name].data = obj;
		list[name].hi = hi;
		list[name].ready(obj, hi);
		list[name].ready = undefined;
   }
}

function xr_frag_insert(name, obj)
{
   var frag_list = xr_frag_list(name);
	xr_list_insert(frag_list, name, undefined, obj);
}

function xr_obj_insert(name, hi, obj)
{
	xr_list_insert(xr_obj, name, hi, obj);
}

function get_common(list, obj_base, name, ready)
{
   var obj_entry = list[name];
   var ready_now;

   if (obj_entry == undefined) {
      var obj_entry = new Object();
      list[name] = obj_entry;
   }

   if (obj_entry.data == undefined) {
      ready_now = false;
   } else {
      ready_now = true;
   }

   if (obj_entry.ready == undefined) {
      obj_entry.ready = function() { };
   }

   var old_ready = obj_entry.ready;
   obj_entry.ready = function(data, hi) {
		old_ready(data, hi); ready(data, hi);
	}

   if (ready_now) {
      setTimeout(function() {
			xr_list_insert(list, name, obj_entry.hi, obj_entry.data);
		});
   } else {
      $.ajax({
         url: obj_base + name,
         dataType: 'script',
         crossDomain: true,
      });
   }
}

function get_frag(name, ready)
{
   get_common(xr_frag_list(name), frag_base, name, ready);
}

function get_obj(name, ready)
{
   get_common(xr_obj, obj_base, name, ready);
}

function get_multi_frag(frags, cb)
{
	var collect = new Object();
	collect.slots = new Array(frags.length);
	collect.count = 0;

	var i;
	for (i = 0; i < frags.length; i++) {
		get_frag(frags[i], (function(cur_i) {
			return function(data) {
				collect.slots[cur_i] = data;
				collect.count++;
				if (collect.count == collect.slots.length)
					cb(collect.slots);
			}
		})(i));
	}
}

function chunk(_lo, _hi, _div)
{
   this.lo = _lo;
   this.hi = _hi;
   this.div = _div;
}

chunk.prototype.ready = function(data)
{
   var i;
   var lno = this.lo;
   var html = '';
   for (i = 0; i < data.length; i++, lno++) {
		var line_id = top_expansion_target.line_id(lno);
      line = '<div class="line" id="'+line_id+'">' +
             '<a class="lno">'+lno+'</a> ' + data[i] + '</div>';
      html += line;
   }
   this.div.append(html);

	if (init_line != undefined && init_line >= this.lo && init_line < lno) {
		var pos = $('#'+top_expansion_target.line_id(init_line)).position();
		$('html,body').scrollTop(pos.top - ($('#header').height() + xr_ch));
	}
}

chunk.prototype.pre = function()
{
   this.div.css("min-height", ''+(xr_ch*(this.hi - this.lo)+'px'));
}

function load_chunk(chunk_desc, hi, i)
{
   var chunk_div = $('<div id="chunk_'+i+'"></div>').appendTo('#main');
   var c = new chunk(chunk_desc.lo, hi, chunk_div);
   c.pre();
   get_frag(chunk_desc.obj, function(data) {c.ready(data);});
}

/* Upper bound binary search on a predicate. Return the index
   of the latest element in arr such that pred(x) */
function ub_bsearch(arr, pred)
{
	var lo = 0;
	var hi = arr.length - 1;
	var mid;
	var best = -1;
	while (lo <= hi) {
		mid = (lo + hi) >> 1;
		if (pred(arr[mid])) {
			best = mid;
			lo = mid + 1;
		} else { hi = mid - 1; }
	}

	return best;
}

/* Like the above, but oppositized */
function lb_bsearch(arr, pred)
{
	var lo = 0;
	var hi = arr.length - 1;
	var mid;
	var best = -1;
	while (lo <= hi) {
		mid = (lo + hi) >> 1;
		if (pred(arr[mid])) {
			best = mid;
			hi = mid - 1;
		} else { lo = mid + 1; }
	}

	return best;
}

function continue_find_tag(frags, ident, cb)
{
	/* find the tag in the first fragment */
	var j = ub_bsearch(frags[0], function(tag) {
		return tag.id < ident;
	});

	var i;
	var found_tags = [];
	if (j != -1) {
		for (i = 0; i < frags.length; i++) {
			for (; j < frags[i].length; j++) {
				if (frags[i][j].id > ident)
					break;
				else if (frags[i][j].id == ident)
					found_tags.push(frags[i][j]);
			}
			j = 0;
		}
	}

	cb(found_tags);
}

function find_tag(ident, cb)
{
   /* Find the tag block */
	var i = ub_bsearch(tags, function(tag) {
		return tag.id < ident;
	});

	var need_frags = [];
	if (i == -1) { i = 0; }
	while (i < tags.length && tags[i].id <= ident) {
		need_frags.push(tags[i].obj);
		i++;
	}

   get_multi_frag(need_frags, function(frags) {
      continue_find_tag(frags, ident, cb);
   });

}

function expansion_target(container, depth)
{
	this.container = container;
	this.line_prefix = 'et'+expansion_target.id;
	expansion_target.id++;
	this.open = {}
	this.depth = depth;

	var this_et = this;
   $(container).on('click', 'div.line', function(ev) {
		click_identifier(ev, this_et);
	});
}

expansion_target.prototype.unload = function(line, ident)
{
	var exp = this.open[line+' '+ident];
	exp.close();
	delete this.open[line+' '+ident];
}

expansion_target.prototype.load = function(line, ident)
{
	var exp = new expansion(line, ident, this);
	this.open[line+' '+ident] = exp;
	find_tag(ident, function(d) { exp.tag_ready(d); });
}

expansion_target.prototype.line_id = function(lno)
{
	return this.line_prefix+'line_'+lno;
}

expansion_target.id = 0;

function click_close_identifier(ev, line, ident, expt)
{
	ev.preventDefault();
	ev.stopPropagation();
	expt.unload(line, ident);
	$(ev.target).off('click');
	$(ev.target).removeClass('select');
}

function click_identifier(ev, expt)
{
   var target = $(ev.target);
   if (!target.is('a.id') || ev.exp_handled)
      return;

	ev.exp_handled = true;
   ev.preventDefault();
	ev.stopPropagation();

	target.addClass('select');

	var line_id = $(ev.currentTarget).attr('id');
   var line = parseInt(line_id.substr(line_id.indexOf('_') + 1));
   var ident_href = target.attr('href');
   var ident = ident_href.substr(1);

   expt.load(line, ident);

	$(ev.target).on('click', function(ev) {
		click_close_identifier(ev, line, ident, expt);
	});
}

function expansion(line, ident, expt)
{
	this.line = line;
	this.ident = ident;
	this.expt = expt;

   var div = $('<div class="expand"/>');
   div.insertAfter('#'+expt.line_id(line));

	this.div = div;
}

function src_window(expand, i, tag_entry, max_len, expt)
{
	this.file = tag_entry.file;

	if (tag_entry.scope_line == undefined)
		this.scope_line = tag_entry.line;
	else
		this.scope_line = tag_entry.scope_line;
	
	this.line = tag_entry.line;
	this.lines = (this.line - this.scope_line) + 10;
	this.selected_line = tag_entry.line;
	this.display_lines = 0;

	this.depth = expt.depth;

	var alt_class =
		(this.depth & 1) ? 'tag_select_bar_even' : 'tag_select_bar_odd';
	var select_bar = $('<div class="tag_select_bar '+alt_class+'">'+
			lno_spacer(this.depth)+'</div>');
	var ts_a = $('<a class="tag_select" href="#">'+tag_entry.type+'</a>')
		ts_a.appendTo(select_bar);
	ts_a.on('click', function(ev) {
			expand.click_type(ev, i);
			});

	var loc_str = this.file + ':' + this.line;
	select_bar.append('<a href="#'+loc_str+'">'+loc_str+'</a>');
	if (tag_entry.scope_name != undefined) {
		var spaces = ' ';
		while (spaces.length + loc_str.length <= max_len)
			spaces += ' ';
		select_bar.append(spaces + tag_entry.scope_name);
	}

	this.select_bar = select_bar;
}

src_window.prototype.expand = function(ev)
{
	this.lines += 10;

	ev.preventDefault();
	ev.stopPropagation();

	var this_wnd = this;
	var cur_height = this.div.height();
	var new_height = cur_height+(10*xr_ch);
	this.div.css('height', ''+cur_height+'px');
	this.div.animate({'height': ''+new_height+'px'}, 100,
		'linear', function() {
			this_wnd.div.css('height','');
			this_wnd.div.css('min-height',''+new_height+'px');
		});

	this.more_line.remove();

	get_obj(this.file + '.xr', function(data, hi) {
		this_wnd.file_obj_ready(data, hi);
	});
}

src_window.prototype.lines_ready = function(lo, frags)
{
	var totals = new Array(frags.length);
	var hi = lo;
	var i;
	for (i = 0; i < frags.length; i++) {
		hi += frags[i].length;
		totals[i] = hi;
	}

	var need_lo = this.scope_line + this.display_lines;
	var need_hi = this.scope_line + this.lines;

	if (lo <= need_lo && hi > need_hi) {
		/* find start frag */
		var start = lo;
		for (i = 0; i < frags.length; i++) {
			if (totals[i] > need_lo)
				break;
			start = totals[i];
		}

		var j = need_lo - start;
		var cur = need_lo;

		var html = '';
		if (this.display_lines == 0) {
			html += '<div class="line">'+lno_spacer(this.depth+1,true)+
				'&nbsp;</div>';
		}

		for (; i < frags.length && cur < need_hi; i++) {
			for (; j < frags[i].length && cur < need_hi; j++, cur++) {
				var line_extra = (cur == this.selected_line) ? ' select' : '';
				var line = '<div class="line'+line_extra+'" id="'+
						this.expt.line_id(cur)+'">' +
						lno_spacer(this.depth)+'<a class="lno">'+cur+'</a> ' +
						frags[i][j] +
						'</div>';
				html += line;
			}
			j = 0;
		}
		this.div.append(html);
		this.display_lines = this.lines;

		var more_bar = $('<a class="more_bar" href="#">VVV</a>');
		more_bar.css('width',
				'' + (this.div.width() - (this.depth+1)*(lno_w+lno_pad))+'px');
		var this_wnd = this;
		more_bar.on('click', function(ev) { this_wnd.expand(ev); });

		var more_line = $('<div class="line">'+
				lno_spacer(this.depth+1,true)+'</div>');
		more_line.append(more_bar);
		this.more_line = more_line;
		this.div.append(more_line);

	}
}

src_window.prototype.file_obj_ready = function(data, hi)
{
	var i;
	if (this.line > hi)
		return;
	
	for (i = 0; i < data.length - 1; i++) {
		if (data[i+1].lo > this.scope_line)
			break;
	}

	var lo = data[i].lo;
	var last = this.scope_line + this.lines;
	if (last >= hi) { this.lines = (hi - 1) - this.scope_line; }
	var need_frags = [];
	for(; i < data.length; i++) {
		if (data[i].lo > last)
			break;
		need_frags.push(data[i].obj);
	}

	var this_wnd = this;
	get_multi_frag(need_frags, function(frags) {
		this_wnd.lines_ready(lo, frags);
	});
}

src_window.prototype.load = function()
{
	var alt_class = (this.depth & 1) ? "src_window_even" : "src_window_odd";
	var div = $('<div class="src_window '+alt_class+'"/>');
	var init_height = ''+(this.lines*xr_ch + 2)+'px';
	div.css('height', init_height);
	div.insertAfter(this.select_bar);
	div.slideDown(100, 'linear', function() {
		div.css({'height':'', 'min-height':init_height});
	});

	this.expt = new expansion_target(div, this.depth+1);
	this.div = div;

	var this_wnd = this;
	get_obj(this.file + '.xr', function(data, hi) {
		this_wnd.file_obj_ready(data, hi);
	});
}

src_window.prototype.unload = function()
{
	var old_div = this.div;
	this.div.css({'min-height':'', 'height':''+this.div.height()+'px'});
	this.div.slideUp(100, 'linear', function() { old_div.remove(); });
	this.div = undefined;
	this.display_lines = 0;
}

var tag_type_order = {'s':0, 'u':1, 't':3, 'e':4, 'f':5, 'c':6, 'm':7,
                      'd':8, 'x':9};

function sort_tags(tag_list)
{
	tag_list.sort(function(t1, t2) {
		var t1_o = tag_type_order[t1.type];
		var t2_o = tag_type_order[t2.type];
		if (t1_o == t2_o) {
			if (t1.file == t2.file)
				return t1.line - t2.line;
			else if (t1.file > t2.file)
				return 1;
			else
				return -1;
		} else {
			return t1_o - t2_o;
		}
	});
}

expansion.prototype.tag_ready = function(tag_list)
{
	var i;

	this.windows = [];

	if (tag_list.length == 0) {
		this.close();
	}

	sort_tags(tag_list);

	/* Get maximum file+line length */
	var max_len = 0;
	for (i = 0; i < tag_list.length; i++) {
		var len = (tag_list[i].file + ':' + tag_list[i].line).length;
		if (len > max_len) max_len = len;
	}

	for (i = 0; i < tag_list.length; i++) {
		var wnd = new src_window(this, i, tag_list[i], max_len, this.expt);
		this.div.append(wnd.select_bar);
		this.windows.push(wnd);
	}

	this.div.slideDown(100, 'linear');
}

expansion.prototype.click_type = function(ev, i)
{
	ev.preventDefault();

	var old = this.current;

	if (old) {
		old.unload();
		this.current = undefined;
	}

	if (this.windows[i] != old) {
		this.current = this.windows[i];
		this.current.load();
	}
}

expansion.prototype.close = function()
{
	var this_exp = this;
	this.div.slideUp(100, 'linear', function() {
		this_exp.div.remove();
	});
}

function lno_spacer(n, no_content)
{
	var bit = (no_content == undefined) ? '-' : '&nbsp;';
	var w = (n-1)*(lno_w+lno_pad) + lno_w;
	return '<div class="lno_space" style="width: '+w+'px;">'+bit+'</div>';
}

function setup_space(n)
{
   var ts_test = $('#ts-test');
   xr_cw = ts_test.width();
   xr_ch = ts_test.height();

   var ts_w = n*xr_cw;
   lno_w = 5*xr_cw;
	lno_pad = xr_ch>>1;
   
   $('#ts_style').replaceWith('<style type="text/css">' +
         'span.ts {display:inline-block; width: '+ts_w+'px; }\n' +
         'a.lno { display: inline-block; min-width: '+lno_w+'px; '
				    +'padding-right: '+lno_pad+'px; }\n' +
			'div.lno_space { padding-right: '+lno_pad+'px; }\n' +
			'a.more_bar  { font-size: 50%; }\n' +
			'.line_height { height: '+xr_ch+'px; }\n' +
         '</style>');
}

function list_dir(data, loc)
{
	var main = $('#main');
	main.empty();

	var i;
	for (i = 0; i < data.length; i++) {
		var ent = data[i];
		var ls_class;
		if (ent[ent.length-1] == '/')
			ls_class = 'ls_folder';
		else
			ls_class = 'ls_file';
		main.append('<div class="'+ls_class+'">'+
				'<a href="#'+loc+ent+'">'+ent+'</a></div>');
	}
}

function remove_search_result(div, height)
{
	var results = $('#search_results');
	var old_height = results.height();
	var new_height = old_height - height;

	div.slideUp(100, 'linear', function() {
		div.remove();
	});
	results.animate({'height':''+new_height+'px'}, 100, 'linear');
}

function search_tag()
{
	var search_str = $('#search_text').val();
	find_tag(search_str, function(found_tags) {
		if (found_tags.length == 0)
			return;

		var result_html = '<div class="search_result">';
		result_html += '<div class="search_header">'+search_str+':</div>';
		result_html += '<div class="search_close">'
		result_html += '<a class="search_close_x" href="#">x</a></div>';
		result_html += '<div class="search_clear"/>';

		sort_tags(found_tags);
		var i;
		for (i = 0; i < found_tags.length; i++) {
			var tag = found_tags[i];
			var loc = tag.file+':'+tag.line;
			result_html += '<div class="tag_result">';
			result_html += tag.type+': <a href="#'+loc+'">'+loc+'</a>';
			result_html += '</div>';
		}

		var result_div = $(result_html);
		var results = $('#search_results');


		result_div.on('click', 'a.search_close_x', function(ev) {
			ev.preventDefault();
			ev.stopPropagation();
			remove_search_result(result_div, this_result_height);
		});

		var old_height = results.height();
		var this_result_height = (found_tags.length + 1) * xr_ch;
		var new_height = old_height + this_result_height;
		results.animate({'height': ''+new_height+'px'}, 100, 'linear');

		result_div.appendTo(results);
	});
}

function load_page()
{
	var loc = url();
	init_line = url_line();

	var header = $('#location');
	header.empty();


	var comp = loc.split('/');
	var cur = '';
	var html = '<a href="#">XR</a>/';
	var i;
	for (i = 0; i < comp.length; i++) {
		var sep = (i != comp.length - 1) ? '/' : '';
		cur += comp[i] + sep;
		html += '<a href="#'+cur+'">'+comp[i]+'</a>'+sep;
	}
	header.html(html);

	if (loc == '' || loc[loc.length-1] == '/') {
		get_obj(loc + 'ls.xr', function(data) {
			list_dir(data, loc);
		});
	} else {
		get_obj(url() + '.xr', function(data, hi) {
			$('#main').empty();

			var i;
			for (i = 0; i < data.length - 1; i++) {
				load_chunk(data[i], data[i+1].lo, i);
			}
			load_chunk(data[i], hi, i);
		});
	}

	document.title = loc;
}

function load_tags()
{
	get_obj('tags.xr', function(data, hi) {
		tags = data;
	});
}

function init_page()
{
	$(window).on('hashchange', function(ev) { load_page(); });

	search_open = 0;
	$('#search_results').css('top', $('#header').height());
	$('#search_form').on('submit', function(ev) {
		search_tag();
		ev.preventDefault();
		ev.stopPropagation();
	});

	setup_space(3);
	top_expansion_target = new expansion_target(document, 1);
   load_page();
	load_tags();
}

$(document).ready(init_page);
