/**
 * PostSidebar Scroll Handler
 * Replicates Medium's Z7 class behavior for showing/hiding the floating sidebar
 * based on scroll position and figure overlap detection.
 */
(function() {
  'use strict';

  // Configuration
  var MIN_VIEWPORT_HEIGHT = 400;
  var MIN_VIEWPORT_WIDTH = 992;

  // State
  var sidebar = null;
  var articleContent = null;
  var figures = [];
  var contentTopOffset = 0;
  var isVisible = false;

  /**
   * Show the sidebar with fade-in animation
   */
  function showSidebar() {
    if (!isVisible && sidebar) {
      sidebar.classList.remove('u-transition--fadeOut300');
      sidebar.classList.add('u-transition--fadeIn300');
      isVisible = true;
    }
  }

  /**
   * Hide the sidebar with fade-out animation
   */
  function hideSidebar() {
    if (isVisible && sidebar) {
      sidebar.classList.remove('u-transition--fadeIn300');
      sidebar.classList.add('u-transition--fadeOut300');
      isVisible = false;
    }
  }

  /**
   * Check if two rectangles overlap
   * @param {DOMRect} rect1 - First rectangle
   * @param {DOMRect} rect2 - Second rectangle
   * @returns {boolean} True if rectangles overlap
   */
  function rectsOverlap(rect1, rect2) {
    return !(
      rect1.right < rect2.left ||
      rect1.left > rect2.right ||
      rect1.bottom < rect2.top ||
      rect1.top > rect2.bottom
    );
  }

  /**
   * Check if sidebar overlaps with any figure element
   * @param {DOMRect} sidebarRect - Sidebar bounding rectangle
   * @returns {boolean} True if sidebar overlaps with any figure
   */
  function checkFigureOverlap(sidebarRect) {
    for (var i = 0; i < figures.length; i++) {
      var figureRect = figures[i].getBoundingClientRect();
      if (rectsOverlap(sidebarRect, figureRect)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Handle scroll and resize events to show/hide sidebar
   */
  function handleScroll() {
    var scrollTop = window.pageYOffset || document.documentElement.scrollTop;
    var viewportHeight = window.innerHeight;
    var viewportWidth = window.innerWidth;
    var sidebarRect = sidebar ? sidebar.getBoundingClientRect() : null;

    // Hide sidebar if:
    // - Viewport is too small
    // - Haven't scrolled past article start
    // - Sidebar overlaps with a figure
    if (
      viewportHeight < MIN_VIEWPORT_HEIGHT ||
      viewportWidth < MIN_VIEWPORT_WIDTH ||
      contentTopOffset > scrollTop ||
      (sidebarRect && checkFigureOverlap(sidebarRect))
    ) {
      hideSidebar();
    } else {
      showSidebar();
    }
  }

  /**
   * Initialize sidebar functionality
   */
  function init() {
    sidebar = document.querySelector('.js-postShareWidget');
    articleContent = document.querySelector('.postArticle-content');

    if (sidebar && articleContent) {
      // Calculate initial content offset
      contentTopOffset = articleContent.getBoundingClientRect().top + window.pageYOffset;

      // Get all figures in the article
      figures = articleContent.querySelectorAll('figure');

      // Start hidden
      sidebar.classList.add('u-transition--fadeOut300');
      sidebar.classList.remove('u-transition--fadeIn300');

      // Attach event listeners
      window.addEventListener('scroll', handleScroll, { passive: true });
      window.addEventListener('resize', handleScroll, { passive: true });

      // Initial check
      handleScroll();
    }
  }

  // Initialize on DOM ready
  document.addEventListener('DOMContentLoaded', init);
})();
