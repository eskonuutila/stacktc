/*==== TCSCCIter: Iterator object for accessing the result transitive closure ==== */

/* Create the iterator object */
TCSCCIter *TCSCCIter_new(TCSCCIter *this, TC* tc, vint from_scc_id, int reversep) {
  if (this == NULL) {
    this = NEW(TCSCCIter);
  }
  SCC *from_scc = tc->scc_table[from_scc_id];
  this->reversep = reversep;
  this->intervals = from_scc->successors;
  if (reversep) {
    this->current_interval_index = this->intervals->interval_count;
    this->interval_limit = -1;
    this->to_scc_id = this->to_scc_limit = 0;
  } else {
    this->current_interval_index = -1;
    this->interval_limit = this->intervals->interval_count;
    this->to_scc_id = this->to_scc_limit = 0;
  }
  return this;
}

/* Get the id of the next SCC */
int TCSCCIter_next(TCSCCIter *this) {
  if (this->reversep) {
    if (this->to_scc_id == this->to_scc_limit) {
      if (--this->current_interval_index <= this->interval_limit)
	return ITER_FINISHED;
      else {
	this->to_scc_id = this->intervals->interval_table[this->current_interval_index].high + 1;
	this->to_scc_limit = this->intervals->interval_table[this->current_interval_index].low;
      }
    }
    return --this->to_scc_id;
  } else {
    if (this->to_scc_id == this->to_scc_limit) {
      if (++this->current_interval_index >= this->interval_limit)
	return ITER_FINISHED;
      else {
	this->to_scc_id = this->intervals->interval_table[this->current_interval_index].low - 1;
	this->to_scc_limit = this->intervals->interval_table[this->current_interval_index].high;
      }
    }
    return ++this->to_scc_id;
  }
}

