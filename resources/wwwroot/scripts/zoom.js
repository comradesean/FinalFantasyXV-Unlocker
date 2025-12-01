/**
 * Medium-Style Image Zoom
 * 1:1 replica of Medium's original zoom functionality from main-base.bundle.js
 *
 * Original function names preserved in comments:
 * - Gya: Handle parent z-indexes
 * - Jya: Cleanup z-indexes and remove classes
 * - Hya: Calculate scale factor
 * - dG: Apply or prepare transform
 * - bG: Scroll handler class (scroll-throttled behavior)
 */
(function() {
  'use strict';

  // Constants (from original: z.AA = 200)
  var SCROLL_THROTTLE_MS = 200;
  var SCROLL_TOLERANCE_PX = 40;

  // State
  var zoomedElement = null;   // The zoomed element (progressiveMedia)
  var originalRect = null;    // Original bounding rect (E4)
  var originalStyles = null;  // Original inline styles (PJ)
  var initialScrollY = null;  // Initial scroll position when zoom opened (UP)

  // Scroll handling state
  var lastThrottleTime = 0;
  var debounceTimer = null;

  /**
   * Handle parent z-indexes by walking up the DOM tree (Gya)
   * Ensures zoomed element appears above siblings
   * @param {Element} element - The zoomed element
   */
  function handleParentZIndexes(element) {
    var body = document.body;
    var elementZIndex = parseInt(window.getComputedStyle(element).zIndex, 10);

    if (!isNaN(elementZIndex)) {
      for (var node = element.parentNode; node && node !== body; node = node.parentNode) {
        var parentZIndex = parseInt(window.getComputedStyle(node).zIndex, 10);
        if (!isNaN(parentZIndex) && parentZIndex < elementZIndex) {
          node.setAttribute('data-zoomable-z-index', node.style.zIndex);
          node.style.zIndex = elementZIndex;
        }
      }
    }
  }

  /**
   * Cleanup z-indexes and remove zoom classes (Jya)
   * @param {Element} element - The previously zoomed element
   */
  function cleanupZIndexes(element) {
    element.classList.remove('zoomable', 'is-static', 'is-zooming');

    var body = document.body;
    for (var node = element.parentNode; node && node !== body; node = node.parentNode) {
      var savedZIndex = node.getAttribute('data-zoomable-z-index');
      if (savedZIndex !== null) {
        node.style.zIndex = savedZIndex;
        node.removeAttribute('data-zoomable-z-index');
      }
    }
  }

  /**
   * Calculate optimal scale factor for the zoomed image (Hya)
   * @param {Element} element - The element being zoomed
   * @param {Object} rect - Original bounding rect
   * @param {Object} viewport - Viewport dimensions
   * @returns {number} Scale factor
   */
  function calculateScaleFactor(element, rect, viewport) {
    var maxWidth = Number(element.getAttribute('data-width')) || Infinity;
    var maxHeight = Number(element.getAttribute('data-height')) || Infinity;

    return Math.min(
      Math.min(viewport.width, maxWidth) / rect.width,
      Math.min(viewport.height, maxHeight) / rect.height
    );
  }

  /**
   * Apply or prepare CSS transform for zoom (dG)
   * @param {boolean} useTransform - Whether to use CSS transform (true) or direct positioning (false)
   */
  function applyTransform(useTransform) {
    var element = zoomedElement;
    var rect = originalRect;
    var viewport = { width: window.innerWidth, height: window.innerHeight };
    var scale = calculateScaleFactor(zoomedElement, rect, viewport);

    // Calculate scaled dimensions
    var scaled = {
      width: rect.width * scale,
      height: rect.height * scale,
      left: rect.left + (rect.width - rect.width * scale) / 2,
      top: rect.top + (rect.height - rect.height * scale) / 2
    };

    var centerX = (viewport.width - scaled.width) / 2;
    var centerY = (viewport.height - scaled.height) / 2;

    if (useTransform) {
      // Use CSS transform for smooth animation
      var translateX = (centerX - scaled.left) / scale;
      var translateY = (centerY - scaled.top) / scale;
      element.style.transform = 'scale(' + scale + ') translate(' + translateX + 'px, ' + translateY + 'px)';

      // Restore original inline styles
      element.style.left = originalStyles.left;
      element.style.top = originalStyles.top;
      element.style.width = originalStyles.width;
      element.style.maxWidth = originalStyles.maxWidth;
      element.style.height = originalStyles.height;
    } else {
      // Use direct positioning (fallback)
      element.style.transform = '';
      element.style.left = (centerX - rect.left - (parseInt(originalStyles.left, 10) || 0)) + 'px';
      element.style.top = (centerY - rect.top - (parseInt(originalStyles.top, 10) || 0)) + 'px';
      element.style.width = scaled.width + 'px';
      element.style.maxWidth = scaled.width + 'px';
      element.style.height = scaled.height + 'px';
    }
  }

  /**
   * Open zoom on target element
   * @param {Element} target - The element to zoom
   */
  function openZoom(target) {
    if (target === zoomedElement) return;

    // Close any existing zoom
    if (zoomedElement) closeZoom();

    // Set initial scroll position (like bG constructor: this.UP = z.jw(this.Ap).top)
    initialScrollY = window.pageYOffset || document.documentElement.scrollTop;

    zoomedElement = target;
    originalRect = target.getBoundingClientRect();

    // Save original inline styles
    originalStyles = {
      width: target.style.width,
      height: target.style.height,
      maxWidth: target.style.maxWidth,
      left: target.style.left,
      top: target.style.top
    };

    // Add zoomable class
    target.classList.add('zoomable');

    // Add is-static if position is static
    if (window.getComputedStyle(target).position === 'static') {
      target.classList.add('is-static');
    }

    // Handle parent z-indexes
    handleParentZIndexes(target);

    // Force reflow before animation
    target.offsetHeight;

    // Add is-zooming class to start animation
    target.classList.add('is-zooming');

    // Add is-zoomableActive to body (hides header/footer)
    document.body.classList.add('is-zoomableActive');

    // Apply transform
    applyTransform(true);
  }

  /**
   * Close the current zoom
   */
  function closeZoom() {
    if (!zoomedElement) return;

    var element = zoomedElement;
    var styles = originalStyles;

    // Clear state
    originalStyles = null;
    zoomedElement = null;
    initialScrollY = null;

    // Remove is-zooming
    element.classList.remove('is-zooming');

    // Remove is-zoomableActive from body
    document.body.classList.remove('is-zoomableActive');

    // Clear transform
    element.style.transform = '';

    // Restore original inline styles
    element.style.left = styles.left;
    element.style.top = styles.top;
    element.style.width = styles.width;
    element.style.maxWidth = styles.maxWidth;
    element.style.height = styles.height;

    // Cleanup after transition completes
    setTimeout(function() {
      cleanupZIndexes(element);
    }, 300);
  }

  /**
   * Check if we should close zoom based on scroll distance
   * Implements original 40px tolerance from z.f.jl
   */
  function checkScrollClose() {
    if (!zoomedElement) return;

    var currentScrollY = window.pageYOffset || document.documentElement.scrollTop;
    if (Math.abs(currentScrollY - initialScrollY) >= SCROLL_TOLERANCE_PX) {
      closeZoom();
    }
  }

  /**
   * Handle scroll events with throttle + debounce
   * Replicates original behavior using both QDa (throttle) and RZ (debounce)
   */
  function handleScroll() {
    if (!zoomedElement) return;

    var now = Date.now();

    // Throttle: fire immediately, then every 200ms (z.Nl/QDa)
    if (now - lastThrottleTime >= SCROLL_THROTTLE_MS) {
      lastThrottleTime = now;
      checkScrollClose();
    }

    // Debounce: also fire 200ms after scrolling stops (z.Ml/RZ)
    if (debounceTimer) clearTimeout(debounceTimer);
    debounceTimer = setTimeout(function() {
      debounceTimer = null;
      checkScrollClose();
    }, SCROLL_THROTTLE_MS);
  }

  /**
   * Initialize zoom functionality
   */
  function init() {
    // Attach click handlers to zoomable images
    var zoomables = document.querySelectorAll('[data-action="zoom"]');
    zoomables.forEach(function(el) {
      el.addEventListener('click', function(e) {
        e.preventDefault();
        e.stopPropagation();
        if (zoomedElement === this) {
          closeZoom();
        } else {
          openZoom(this);
        }
      });
    });

    // Close on click outside
    document.addEventListener('click', function(e) {
      if (zoomedElement && !zoomedElement.contains(e.target)) {
        closeZoom();
      }
    }, true);

    // Close on escape key
    document.addEventListener('keyup', function(e) {
      if (e.key === 'Escape' || e.keyCode === 27) {
        closeZoom();
      }
    });

    // Close on scroll (with throttle + debounce + 40px tolerance)
    window.addEventListener('scroll', handleScroll, { passive: true });
  }

  // Initialize on DOM ready
  document.addEventListener('DOMContentLoaded', init);
})();
