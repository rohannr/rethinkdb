#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TO_GLOBAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TO_GLOBAL_HPP_

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "http/json/json_adapter.hpp"

class remote_issue_t : public global_issue_t {
public:
    remote_issue_t(const clone_ptr_t<local_issue_t> &_underlying, machine_id_t _source) :
        underlying(_underlying), source(_source) { }

    std::string get_description() const {
        return "On machine " + uuid_to_str(source) + ": " + underlying->get_description();
    }

    cJSON *get_json_description() {
        cJSON *res = underlying->get_json_description();
        cJSON_AddItemToObject(res, "location", render_as_json(&source, 0));

        return res;
    }

    remote_issue_t *clone() const {
        return new remote_issue_t(underlying, source);
    }

    clone_ptr_t<local_issue_t> underlying;
    machine_id_t source;
};

class remote_issue_collector_t : public global_issue_tracker_t {
public:
    remote_issue_collector_t(
            const clone_ptr_t<watchable_t<std::map<peer_id_t, std::list<clone_ptr_t<local_issue_t> > > > > &_issues_view,
            const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &_machine_id_translation_table) :
        issues_view(_issues_view), machine_id_translation_table(_machine_id_translation_table) { }

    std::list<clone_ptr_t<global_issue_t> > get_issues() {
        std::list<clone_ptr_t<global_issue_t> > l;
        std::map<peer_id_t, std::list<clone_ptr_t<local_issue_t> > > issues_by_peer = issues_view->get();
        std::map<peer_id_t, machine_id_t> translation_table = machine_id_translation_table->get();
        for (std::map<peer_id_t, std::list<clone_ptr_t<local_issue_t> > >::iterator it = issues_by_peer.begin(); it != issues_by_peer.end(); it++) {
            std::map<peer_id_t, machine_id_t>::const_iterator tt_it = translation_table.find(it->first);
            if (tt_it == translation_table.end()) {
                /* This is unexpected. Did they disconnect after we got the
                issues list but before we got the translation table? In any
                case, do something safe. */
                continue;
            }
            std::list<clone_ptr_t<local_issue_t> > issues = it->second;
            for (std::list<clone_ptr_t<local_issue_t> >::iterator jt = issues.begin(); jt != issues.end(); jt++) {
                l.push_back(clone_ptr_t<global_issue_t>(new remote_issue_t(*jt, tt_it->second)));
            }
        }
        return l;
    }

private:
    clone_ptr_t<watchable_t<std::map<peer_id_t, std::list<clone_ptr_t<local_issue_t> > > > > issues_view;
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_translation_table;
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TO_GLOBAL_HPP_ */
