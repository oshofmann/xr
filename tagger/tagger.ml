open Cil
open Printf
open Machine

let rec ___str_startswith str prefix i len =
	if (i = len) then len
	else (
		if (str.[i] = prefix.[i]) then (___str_startswith str prefix (i+1) len)
		else 0
	)

let __str_startswith str strlen prefix =
	let len = String.length prefix in
	if (len > strlen) then 0
	else ___str_startswith str prefix 0 len

let rec _str_startswith str strlen prefix_list =
	match prefix_list with
	| [] -> 0
	| prefix :: prefix_list ->
		let res = __str_startswith str strlen prefix in
		if (res = 0) then _str_startswith str strlen prefix_list
		else res

let str_startswith str prefix_list =
	let strlen = String.length str in _str_startswith str strlen prefix_list

let name_missing name =
	(str_startswith name ["__anon"; "__annon"; "___missing_field_name"]) != 0

let not_anon_name name =
	if name_missing name then None
	else Some name

let path_sep_r = Str.regexp_string Filename.dir_sep

let rec _canon_path path i =
	let (comp_end, consume, built_path) =
		(try
			let comp_end = Str.search_forward path_sep_r path i in
			let (consume, built_path) = _canon_path path (comp_end+1) in
			(comp_end, consume, built_path)
		with Not_found ->
			(String.length path, 0, [])
		) in
	let comp = String.sub path i (comp_end - i) in
	match comp with
	| "." -> (consume, built_path)
	| ".." -> (consume+1, built_path)
	| comp -> 
		if (consume > 0) then ((consume - 1), built_path)
		else (0, (comp :: built_path))

let rec _dotdot n =
	match n with
	| 0 -> []
	| n -> ".." :: _dotdot (n-1)

let canon_path path =
	let (consume, comp_list) = _canon_path path 0 in
	let new_path = String.concat Filename.dir_sep comp_list in
	let dotdot = String.concat Filename.dir_sep (_dotdot consume) in
	dotdot ^ new_path

let rec _get_strip_list i len =
	if (i = len) then [] else (Sys.argv.(i) :: _get_strip_list (i+1) len)

let get_strip_list =
	let len = Array.length Sys.argv in
	_get_strip_list 2 len

let strip_list = get_strip_list

let strip_filename name = 
	let name = canon_path name in
	let strip_len = str_startswith name strip_list in
	String.sub name strip_len ((String.length name) - strip_len)

let kv_s k v =
	printf "\"%s\":\"%s\",\t" k v

let kv_d k v =
	printf "\"%s\":%d,\t" k v

let kv_c k v =
	printf "\"%s\":\"%c\",\t" k v

let output_tag name tag_type file line scope_line_opt scope_name_opt =
	kv_s "id" name; kv_s "file" (strip_filename file);
	kv_d "line" line; kv_c "type" tag_type; 
	(match scope_line_opt with
	| Some scope_line -> kv_d "scope_line" scope_line 
	| None -> ());
	(match scope_name_opt with
	| Some scope_name -> kv_s "scope_name" scope_name
	| None -> ());
	print_string "\n"

class fn_body_visitor = object(self)
	inherit nopCilVisitor

	val mutable fn_line = None
	val mutable fn_name = None

	method vinst (i : instr) =
		match i with
		| Call (_, Lval (Var v, _), _, loc) ->
			output_tag v.vname 'c' loc.file loc.line fn_line fn_name;
			DoChildren
		| _ -> DoChildren

	method set_fn_line l = fn_line <- l
	method set_fn_name n = fn_name <- n
end

let rec output_fields fields scope_line scope_name =
	match fields with
	| [] -> ()
	| field :: fields ->
		(if not (name_missing field.fname) then
			output_tag field.fname 'm' field.floc.file field.floc.line scope_line
				scope_name
		else ());
		(match field.ftype with
		| TComp (comp, _) ->
			if (name_missing comp.cname) then
				output_fields comp.cfields scope_line scope_name
			else ()
		| _ -> ());
		output_fields fields scope_line scope_name

let rec output_enums items scope_line scope_name =
	match items with
	| [] -> ()
	| (name, _, loc) :: items ->
		output_tag name 'e' loc.file loc.line scope_line scope_name;
		output_enums items scope_line scope_name

class tagger_visitor = object(self)
   inherit nopCilVisitor

	val body_visit = new fn_body_visitor

   method vglob (g : global) =
      match g with
      | GFun (dec, loc) ->
			output_tag dec.svar.vname 'f' loc.file loc.line None None;
			body_visit#set_fn_line (Some loc.line);
			body_visit#set_fn_name (Some dec.svar.vname);
			ignore (visitCilFunction (body_visit :> cilVisitor) dec);
         SkipChildren
		| GCompTag (comp, loc) ->
			let comp_type = if (comp.cstruct) then "struct " else "union " in
			if not (name_missing comp.cname) then (
				output_tag comp.cname comp_type.[0] loc.file loc.line None None;
				output_fields comp.cfields (Some loc.line)
					(Some (comp_type ^ comp.cname))
			);
			SkipChildren
		| GEnumTag (enum, loc) ->
			if not (name_missing enum.ename) then
				output_tag enum.ename 't' loc.file loc.line None None
			else ();
			output_enums enum.eitems (Some loc.line) (not_anon_name enum.ename);
			SkipChildren
		| GType (tinfo, loc) ->
			output_tag tinfo.tname 't' loc.file loc.line None None;
			(match tinfo.ttype with
			| TComp (comp, _) ->
				if (name_missing comp.cname) then
					output_fields comp.cfields (Some loc.line) (Some tinfo.tname)
				else ()
			| _ -> ());
			SkipChildren
		| GVar (vinfo, _, loc) ->
			output_tag vinfo.vname 'v' loc.file loc.line None None;
			(match vinfo.vtype with
			| TComp (comp, _) ->
				if (name_missing comp.cname) then
					output_fields comp.cfields (Some loc.line) (Some vinfo.vname)
				else ()
			| _ -> ());
			SkipChildren
      | _ -> SkipChildren
end
      
let () =
	let machineModel =
      try (Some (Machdepenv.modelParse (machine_str)))
      with Not_found -> None in
   Cil.envMachine := machineModel;

   let filename = Sys.argv.(1) in
   let parsed = (Frontc.parse filename) () in
   (* Rmtmps.removeUnusedTemps parsed; *)
   visitCilFileSameGlobals (new tagger_visitor) parsed
