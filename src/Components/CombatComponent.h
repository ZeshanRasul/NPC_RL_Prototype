#pragma once

// Plain-data vitals struct shared by Player and Enemy.
// Entity-specific callbacks (OnDeath, OnHit animations) stay on the entity;
// this component owns the numbers those callbacks read and write.
struct CombatComponent {
    float health             = 100.0f;
    float maxHealth          = 100.0f;
    float accuracy           = 100.0f;  // percent hit chance (0–100)
    float shootCooldown      = 0.0f;
    float shootAudioCooldown = 0.0f;
    bool  isDead             = false;
    bool  hasShot            = false;
    bool  hasHit             = false;

    void ApplyDamage(float amount)
    {
        health -= amount;
        if (health <= 0.0f)
        {
            health = 0.0f;
            isDead = true;
        }
    }

    bool IsAlive() const { return !isDead; }

    void Reset()
    {
        health           = maxHealth;
        isDead           = false;
        hasShot          = false;
        hasHit           = false;
        shootCooldown    = 0.0f;
        shootAudioCooldown = 0.0f;
    }
};
