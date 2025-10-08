const pressedKeys = new Set();

document.addEventListener("keydown", (e) => {
    pressedKeys.add(e.key.toLowerCase());
});

document.addEventListener("keyup", (e) => {
    pressedKeys.delete(e.key.toLowerCase());
});

setInterval(() => {
    const keys = [...pressedKeys].filter(k => "wasd".includes(k)).join("");
    fetch(`/keys?pressed=${keys}`)
            .then(response => response.text())
            .catch(err => {
                console.error("Fehler beim fetch:", err);
            });
}, 75); // alle 75ms senden