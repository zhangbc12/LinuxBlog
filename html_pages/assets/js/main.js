(function ($) {
	"use strict";


/* search overlay */
 $('#close-btn').on('click', function () {
    $('#search-overlay').fadeOut();
    $('#search-btn').show();
  });
   $('#search-btn').on('click', function () {
    $(this).hide();
    $('#search-overlay').fadeIn();
  });

/* scrollUp */
    jQuery.scrollUp({
    scrollName: 'scrollUp', 
    topDistance: '300', 
    topSpeed: 4000, 
    animation: 'fade', 
	animationInSpeed: 1000, 
    animationOutSpeed: 1000, 
    scrollText: '<i class="fa fa-long-arrow-up" aria-hidden="true"></i>', 
    activeOverlay: false, 
  });

	// Fit ideos
	var $allVideos = $(".videoWrapper iframe"),
		$fluidEl = $("body");
	$allVideos.each(function() {

	  $(this)
		.data('aspectRatio', this.height / this.width)
		.removeAttr('height')
		.removeAttr('width');

	});
	$(window).resize(function() {

	  var newWidth = $fluidEl.width();
	  $allVideos.each(function() {

		var $el = $(this);
		$el
		  .width(newWidth)
		  .height(newWidth * $el.data('aspectRatio'));

	  });

	}).resize();
























}(jQuery));
 
$(window).on('load',function(){

/*  Preloader js*/
        var preLoder = $(".overlay-loader");
        preLoder.fadeOut(1000);
       
    });    
		
/*  Sidebar panel */	
function openNav() {
  document.getElementById("mySidepanel").style.right = "0px";
}

function closeNav() {
  document.getElementById("mySidepanel").style.right = "-500px";
}         