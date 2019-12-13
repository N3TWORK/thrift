struct Entry {
	t_struct *t;
	vector<t_field*> fields; // instances of LocalizedString
	vector<t_field*> lockeys; // fields tagged with "lockey"

	bool operator<(const Entry &that) const { return t->get_name() < that.t->get_name(); }
};

void write_loc_index(string fname) {
	const vector<t_struct*>& v = g_program->get_structs();
	vector<Entry> entries;
	for(int i = 0; i < v.size(); i++) {
		Entry e;
		e.t = v[i];
		for(int j = 0; j < e.t->get_sorted_members().size(); j++) {
			t_field *f = e.t->get_sorted_members()[j];
			if(f->get_type()->get_name() == "LocalizedString") {
				e.fields.push_back(f);
			}
			if(f->annotations_.count("lockey") > 0) {
				e.lockeys.push_back(f);
			}
		}
		if(!e.fields.empty() || !e.lockeys.empty()) {
			entries.push_back(e);
		}
	}
	sort(entries.begin(), entries.end());
	FILE *f = fopen(fname.c_str(), "w");
	if(!f) {
		fprintf(stderr, "could not open locindex file '%s'\n", fname.c_str());
		exit(1);
	}
	fprintf(f, "{\n");
	for(int i = 0; i != entries.size(); ++i) {
		const Entry &e = entries[i];
		fprintf(f, "	\"%s\": {\n", e.t->get_name().c_str());
		fprintf(f, "		\"fields\": {\n");
		for(int j = 0; j < e.fields.size(); j++) {
			fprintf(f, "			\"%s\": {\n", e.fields[j]->get_name().c_str());
			fprintf(f, "				\"slot\": \"%d\"\n", e.fields[j]->get_key());
			fprintf(f, "			}%s\n", (j < e.fields.size() - 1) ? "," : "");
		}
		fprintf(f, "		},\n");
		fprintf(f, "		\"lockeys\": [\n");
		for(int j = 0; j < e.lockeys.size(); j++) {
			fprintf(f, "			\"%s\"%s\n", e.lockeys[j]->get_name().c_str(), (j < e.lockeys.size() - 1) ? "," : "");
		}
		fprintf(f, "		]\n");
		fprintf(f, "	}%s\n", (i < entries.size() - 1 ? "," : ""));
	}
	fprintf(f, "}\n");
	fclose(f);
}
