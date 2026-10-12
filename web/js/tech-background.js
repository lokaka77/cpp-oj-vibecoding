(function () {
    "use strict";

    function init(canvasId) {
        const canvas = document.getElementById(canvasId);
        if (!canvas || !canvas.getContext) {
            return;
        }

        const context = canvas.getContext("2d");
        const reduceMotion = window.matchMedia(
            "(prefers-reduced-motion: reduce)"
        ).matches;

        let width = 0;
        let height = 0;
        let nodes = [];
        let frameId = 0;

        function buildNodes() {
            const nodeCount = Math.min(
                72,
                Math.max(32, Math.floor((width * height) / 28000))
            );

            nodes = Array.from({ length: nodeCount }, function () {
                return {
                    x: Math.random() * width,
                    y: Math.random() * height,
                    vx: (Math.random() - 0.5) * 0.16,
                    vy: (Math.random() - 0.5) * 0.16,
                    radius: Math.random() * 1.4 + 0.7,
                    accent: Math.random() < 0.12
                };
            });
        }

        function resize() {
            const ratio = Math.min(window.devicePixelRatio || 1, 2);
            width = window.innerWidth;
            height = window.innerHeight;
            canvas.width = Math.floor(width * ratio);
            canvas.height = Math.floor(height * ratio);
            context.setTransform(ratio, 0, 0, ratio, 0, 0);
            buildNodes();
            draw();
        }

        function drawGrid() {
            const spacing = 48;
            context.beginPath();
            context.strokeStyle = "rgba(29, 42, 56, 0.42)";
            context.lineWidth = 1;

            for (let x = 0; x <= width; x += spacing) {
                context.moveTo(x, 0);
                context.lineTo(x, height);
            }

            for (let y = 0; y <= height; y += spacing) {
                context.moveTo(0, y);
                context.lineTo(width, y);
            }

            context.stroke();
        }

        function drawConnections() {
            for (let i = 0; i < nodes.length; i += 1) {
                for (let j = i + 1; j < nodes.length; j += 1) {
                    const dx = nodes[i].x - nodes[j].x;
                    const dy = nodes[i].y - nodes[j].y;
                    const distance = Math.sqrt(dx * dx + dy * dy);
                    if (distance > 138) {
                        continue;
                    }

                    const alpha = (1 - distance / 138) * 0.16;
                    context.beginPath();
                    context.strokeStyle =
                        "rgba(38, 208, 206, " + alpha.toFixed(3) + ")";
                    context.lineWidth = 1;
                    context.moveTo(nodes[i].x, nodes[i].y);
                    context.lineTo(nodes[j].x, nodes[j].y);
                    context.stroke();
                }
            }
        }

        function drawNodes() {
            nodes.forEach(function (node) {
                context.beginPath();
                context.fillStyle = node.accent
                    ? "rgba(242, 184, 75, 0.72)"
                    : "rgba(38, 208, 206, 0.62)";
                context.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
                context.fill();
            });
        }

        function updateNodes() {
            nodes.forEach(function (node) {
                node.x += node.vx;
                node.y += node.vy;

                if (node.x < 0 || node.x > width) {
                    node.vx *= -1;
                }
                if (node.y < 0 || node.y > height) {
                    node.vy *= -1;
                }
            });
        }

        function draw() {
            context.clearRect(0, 0, width, height);
            drawGrid();
            drawConnections();
            drawNodes();

            if (!reduceMotion) {
                updateNodes();
                frameId = window.requestAnimationFrame(draw);
            }
        }

        window.addEventListener("resize", function () {
            window.cancelAnimationFrame(frameId);
            resize();
        });

        resize();
    }

    window.CodeHunterBackground = {
        init: init
    };
})();
