/**
 * Pagination Module
 */

import state from './state.js';
import { loadResultsTable } from './results.js';

let currentPage = 1;
const itemsPerPage = 20;

function previousPage() {
  if (currentPage > 1) {
    currentPage--;
    loadResultsTable();
    updatePaginationInfo();
  }
}

function nextPage() {
  const maxPage = Math.ceil(state.results.length / itemsPerPage);
  if (currentPage < maxPage) {
    currentPage++;
    loadResultsTable();
    updatePaginationInfo();
  }
}

function updatePaginationInfo() {
  const maxPage = Math.ceil(state.results.length / itemsPerPage);
  document.getElementById('paginationInfo').textContent = `صفحه ${currentPage} از ${maxPage}`;
  
  document.getElementById('prevBtn').disabled = currentPage === 1;
  document.getElementById('nextBtn').disabled = currentPage === maxPage;
}

export { previousPage, nextPage, updatePaginationInfo };