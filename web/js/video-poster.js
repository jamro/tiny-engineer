(function () {
  function activate(frame) {
    if (frame.dataset.playing === "1") return;
    const id = frame.dataset.youtubeId;
    if (!id) return;

    frame.dataset.playing = "1";

    const iframe = document.createElement("iframe");
    iframe.src =
      "https://www.youtube-nocookie.com/embed/" +
      encodeURIComponent(id) +
      "?autoplay=1&rel=0&modestbranding=1";
    iframe.title = "Tiny Engineer demo";
    iframe.allow =
      "accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share";
    iframe.allowFullscreen = true;
    iframe.referrerPolicy = "strict-origin-when-cross-origin";

    frame.replaceChildren(iframe);
    iframe.focus();
  }

  document.querySelectorAll(".video-frame[data-youtube-id]").forEach(function (frame) {
    const button = frame.querySelector(".video-poster");
    if (!button) return;
    button.addEventListener("click", function () {
      activate(frame);
    });
  });
})();
